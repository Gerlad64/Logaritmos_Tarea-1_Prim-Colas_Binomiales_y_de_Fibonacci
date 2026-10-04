/*
 * sysinfo.c - Detección de hardware y sistema operativo para benchmarking
 *
 * Plataformas: Linux, WSL1/WSL2, macOS (Intel y Apple Silicon), Windows
 *
 * Detecta: SO, arquitectura, CPU (nombre, núcleos físicos/lógicos),
 *          RAM total/disponible, cachés L1d/L1i/L2/L3 y tamaño de línea.
 *
 * Los tamaños de caché son los de UNA instancia (p. ej. el L2 de un núcleo,
 * el L3 compartido), no la suma de todo el chip.
 *
 * Compilar:
 *   Linux / WSL / macOS : cc -O2 -Wall -o sysinfo sysinfo.c
 *   Windows (MinGW)     : gcc -O2 -o sysinfo.exe sysinfo.c -ladvapi32
 *   Windows (MSVC)      : cl /O2 sysinfo.c
 *
 * Uso:
 *   ./sysinfo          salida legible
 *   ./sysinfo --json   salida JSON (útil para guardar junto a resultados)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(_WIN32)
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0601
  #endif
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "advapi32.lib")
  #endif
#elif defined(__APPLE__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #include <sys/utsname.h>
  #include <unistd.h>
#elif defined(__linux__)
  #include <sys/utsname.h>
  #include <unistd.h>
#else
  #error "Plataforma no soportada (se espera Linux, macOS o Windows)"
#endif

#define SET(dst, ...) snprintf((dst), sizeof(dst), __VA_ARGS__)

typedef struct {
    char     os_name[64];     /* "Linux", "Linux (WSL2)", "macOS", "Windows" */
    char     os_detail[256];  /* distro / versión / build */
    char     arch[96];
    char     cpu_name[256];
    int      cores_physical;
    int      cores_logical;
    uint64_t ram_total;       /* bytes */
    uint64_t ram_avail;       /* bytes, 0 = desconocido */
    uint64_t l1d, l1i, l2, l3;/* bytes por instancia, 0 = desconocido */
    uint32_t cache_line;      /* bytes, 0 = desconocido */
} SysInfo;

/* ------------------------------------------------------------------ */
/* Utilidades comunes                                                  */
/* ------------------------------------------------------------------ */

static void fmt_bytes(uint64_t b, char *out, size_t n)
{
    if (b == 0)                    snprintf(out, n, "N/D");
    else if (b >= (1ULL << 30))    snprintf(out, n, "%.2f GiB", (double)b / (1ULL << 30));
    else if (b >= (1ULL << 20))    snprintf(out, n, "%.2f MiB", (double)b / (1ULL << 20));
    else if (b >= (1ULL << 10))    snprintf(out, n, "%.0f KiB", (double)b / (1ULL << 10));
    else                           snprintf(out, n, "%llu B", (unsigned long long)b);
}

/* ------------------------------------------------------------------ */
/* Linux / WSL                                                         */
/* ------------------------------------------------------------------ */
#if defined(__linux__)

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' ||
                     s[n-1] == ' '  || s[n-1] == '\t'))
        s[--n] = 0;
    return s;
}

static int read_file(const char *path, char *buf, size_t n)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(buf, (int)n, f)) { fclose(f); return 0; }
    fclose(f);
    buf[strcspn(buf, "\r\n")] = 0;
    return 1;
}

/* "32K", "1024K", "8M" -> bytes */
static uint64_t parse_size_str(const char *s)
{
    char *end;
    unsigned long long v = strtoull(s, &end, 10);
    while (*end == ' ') end++;
    switch (*end) {
        case 'K': case 'k': v <<= 10; break;
        case 'M': case 'm': v <<= 20; break;
        case 'G': case 'g': v <<= 30; break;
        default: break;
    }
    return v;
}

#define MAX_CORES 8192
static long seen_cores[MAX_CORES];
static int  n_seen_cores = 0;

static void add_core(int phys, int core)
{
    long key = (long)phys * 100000L + core;
    for (int i = 0; i < n_seen_cores; i++)
        if (seen_cores[i] == key) return;
    if (n_seen_cores < MAX_CORES) seen_cores[n_seen_cores++] = key;
}

static void detect_linux(SysInfo *si)
{
    char buf[512], line[512];
    struct utsname u;
    int wsl = 0;
    FILE *f;

    uname(&u);
    SET(si->arch, "%s", u.machine);

    /* WSL: el kernel se identifica como "...microsoft..." */
    if (read_file("/proc/sys/kernel/osrelease", buf, sizeof buf) &&
        (strstr(buf, "microsoft") || strstr(buf, "Microsoft")))
        wsl = strstr(buf, "WSL2") ? 2 : 1;

    if      (wsl == 2) SET(si->os_name, "Linux (WSL2)");
    else if (wsl == 1) SET(si->os_name, "Linux (WSL1)");
    else               SET(si->os_name, "Linux");

    /* Distribución */
    char pretty[200] = "";
    f = fopen("/etc/os-release", "r");
    if (f) {
        while (fgets(line, sizeof line, f)) {
            if (!strncmp(line, "PRETTY_NAME=", 12)) {
                char *v = trim(line + 12);
                size_t n = strlen(v);
                if (n >= 2 && v[0] == '"') { v[n-1] = 0; v++; }
                SET(pretty, "%s", v);
                break;
            }
        }
        fclose(f);
    }
    SET(si->os_detail, "%s, kernel %s",
        pretty[0] ? pretty : "distro desconocida", u.release);

    /* CPU */
    f = fopen("/proc/cpuinfo", "r");
    if (f) {
        int phys = 0, core = -1, have_core = 0;
        while (fgets(line, sizeof line, f)) {
            char *colon = strchr(line, ':');
            if (!colon) continue;
            *colon = 0;
            char *key = trim(line);
            char *val = trim(colon + 1);

            if (!strcmp(key, "processor")) {
                if (si->cores_logical > 0 && have_core) add_core(phys, core);
                si->cores_logical++;
                phys = 0; core = -1; have_core = 0;
            } else if (!strcmp(key, "model name") && !si->cpu_name[0]) {
                SET(si->cpu_name, "%s", val);
            } else if ((!strcmp(key, "Hardware") || !strcmp(key, "Model") ||
                        !strcmp(key, "cpu model")) && !si->cpu_name[0]) {
                SET(si->cpu_name, "%s", val);          /* ARM / PowerPC */
            } else if (!strcmp(key, "physical id")) {
                phys = atoi(val);
            } else if (!strcmp(key, "core id")) {
                core = atoi(val); have_core = 1;
            }
        }
        if (si->cores_logical > 0 && have_core) add_core(phys, core);
        fclose(f);
    }
    if (si->cores_logical == 0) {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        si->cores_logical = n > 0 ? (int)n : 1;
    }
    /* Sin "core id" (ARM, algunos WSL): asumimos 1 hilo por núcleo */
    si->cores_physical = n_seen_cores > 0 ? n_seen_cores : si->cores_logical;
    if (!si->cpu_name[0]) SET(si->cpu_name, "Desconocido (%s)", si->arch);

    /* RAM */
    f = fopen("/proc/meminfo", "r");
    if (f) {
        while (fgets(line, sizeof line, f)) {
            unsigned long long kb;
            if (sscanf(line, "MemTotal: %llu kB", &kb) == 1)
                si->ram_total = kb * 1024ULL;
            else if (sscanf(line, "MemAvailable: %llu kB", &kb) == 1)
                si->ram_avail = kb * 1024ULL;
        }
        fclose(f);
    }
    if (si->ram_total == 0) {
        long pages = sysconf(_SC_PHYS_PAGES), psz = sysconf(_SC_PAGESIZE);
        if (pages > 0 && psz > 0) si->ram_total = (uint64_t)pages * psz;
    }

    /* Cachés (sysfs, cpu0) */
    for (int i = 0; i < 16; i++) {
        char p[160], lv[32], ty[32], sz[32], ln[32];
        snprintf(p, sizeof p, "/sys/devices/system/cpu/cpu0/cache/index%d/level", i);
        if (!read_file(p, lv, sizeof lv)) break;
        snprintf(p, sizeof p, "/sys/devices/system/cpu/cpu0/cache/index%d/type", i);
        if (!read_file(p, ty, sizeof ty)) continue;
        snprintf(p, sizeof p, "/sys/devices/system/cpu/cpu0/cache/index%d/size", i);
        if (!read_file(p, sz, sizeof sz)) continue;

        int level = atoi(lv);
        uint64_t bytes = parse_size_str(sz);
        if      (level == 1 && !strcmp(ty, "Data"))        si->l1d = bytes;
        else if (level == 1 && !strcmp(ty, "Instruction")) si->l1i = bytes;
        else if (level == 2)                               si->l2  = bytes;
        else if (level == 3)                               si->l3  = bytes;

        snprintf(p, sizeof p,
                 "/sys/devices/system/cpu/cpu0/cache/index%d/coherency_line_size", i);
        if (!si->cache_line && read_file(p, ln, sizeof ln))
            si->cache_line = (uint32_t)atoi(ln);
    }

    /* Respaldo con glibc si sysfs no expone la caché */
#ifdef _SC_LEVEL1_DCACHE_SIZE
    if (!si->l1d) { long v = sysconf(_SC_LEVEL1_DCACHE_SIZE); if (v > 0) si->l1d = (uint64_t)v; }
    if (!si->l1i) { long v = sysconf(_SC_LEVEL1_ICACHE_SIZE); if (v > 0) si->l1i = (uint64_t)v; }
    if (!si->l2)  { long v = sysconf(_SC_LEVEL2_CACHE_SIZE);  if (v > 0) si->l2  = (uint64_t)v; }
    if (!si->l3)  { long v = sysconf(_SC_LEVEL3_CACHE_SIZE);  if (v > 0) si->l3  = (uint64_t)v; }
    if (!si->cache_line) {
        long v = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
        if (v > 0) si->cache_line = (uint32_t)v;
    }
#endif
}

/* ------------------------------------------------------------------ */
/* macOS                                                               */
/* ------------------------------------------------------------------ */
#elif defined(__APPLE__)

/* Apple solo corre en little-endian, así que un buffer de 8 bytes
 * inicializado en 0 funciona tanto para valores de 4 como de 8 bytes. */
static uint64_t sysctl_u64(const char *name)
{
    uint64_t v = 0;
    size_t len = sizeof v;
    if (sysctlbyname(name, &v, &len, NULL, 0) != 0) return 0;
    return v;
}

static int sysctl_str(const char *name, char *out, size_t n)
{
    size_t len = n;
    if (sysctlbyname(name, out, &len, NULL, 0) != 0) { out[0] = 0; return 0; }
    return 1;
}

static void detect_macos(SysInfo *si)
{
    struct utsname u;
    char ver[64] = "";

    uname(&u);
    SET(si->arch, "%s", u.machine);
    if (sysctl_u64("sysctl.proc_translated") == 1)
        snprintf(si->arch + strlen(si->arch), sizeof(si->arch) - strlen(si->arch),
                 " (Rosetta)");

    SET(si->os_name, "macOS");
    sysctl_str("kern.osproductversion", ver, sizeof ver);
    SET(si->os_detail, "macOS %s, Darwin %s", ver[0] ? ver : "?", u.release);

    if (!sysctl_str("machdep.cpu.brand_string", si->cpu_name, sizeof si->cpu_name))
        SET(si->cpu_name, "Desconocido (%s)", si->arch);

    si->cores_physical = (int)sysctl_u64("hw.physicalcpu");
    si->cores_logical  = (int)sysctl_u64("hw.logicalcpu");
    if (si->cores_logical <= 0) {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        si->cores_logical = n > 0 ? (int)n : 1;
    }
    if (si->cores_physical <= 0) si->cores_physical = si->cores_logical;

    si->ram_total  = sysctl_u64("hw.memsize");
    si->l1d        = sysctl_u64("hw.l1dcachesize");
    si->l1i        = sysctl_u64("hw.l1icachesize");
    si->l2         = sysctl_u64("hw.l2cachesize");
    si->l3         = sysctl_u64("hw.l3cachesize");
    si->cache_line = (uint32_t)sysctl_u64("hw.cachelinesize");
}

/* ------------------------------------------------------------------ */
/* Windows                                                             */
/* ------------------------------------------------------------------ */
#elif defined(_WIN32)

static void reg_str(const char *subkey, const char *name, char *out, DWORD n)
{
    HKEY h;
    DWORD type = 0, sz = n - 1;
    out[0] = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0,
                      KEY_READ | KEY_WOW64_64KEY, &h) != ERROR_SUCCESS)
        return;
    if (RegQueryValueExA(h, name, NULL, &type, (LPBYTE)out, &sz) == ERROR_SUCCESS &&
        type == REG_SZ)
        out[sz < n ? sz : n - 1] = 0;
    else
        out[0] = 0;
    RegCloseKey(h);
}

static char *trim_ws(char *s)
{
    while (*s == ' ') s++;
    size_t n = strlen(s);
    while (n > 0 && s[n-1] == ' ') s[--n] = 0;
    return s;
}

static void detect_windows(SysInfo *si)
{
    SYSTEM_INFO sys;
    GetNativeSystemInfo(&sys);
    switch (sys.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: SET(si->arch, "x86_64"); break;
        case PROCESSOR_ARCHITECTURE_ARM64: SET(si->arch, "arm64");  break;
        case PROCESSOR_ARCHITECTURE_INTEL: SET(si->arch, "x86");    break;
        case PROCESSOR_ARCHITECTURE_ARM:   SET(si->arch, "arm");    break;
        default:                           SET(si->arch, "desconocida"); break;
    }

    /* SO: el registro es más fiable que GetVersionEx (que miente) */
    const char *cv = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    char prod[128], disp[64], build[32];
    reg_str(cv, "ProductName", prod, sizeof prod);
    reg_str(cv, "DisplayVersion", disp, sizeof disp);
    if (!disp[0]) reg_str(cv, "ReleaseId", disp, sizeof disp);
    reg_str(cv, "CurrentBuildNumber", build, sizeof build);

    /* Windows 11 sigue diciendo "Windows 10" en ProductName */
    if (atoi(build) >= 22000) {
        char *p = strstr(prod, "Windows 10");
        if (p) p[9] = '1', p[10] = '1';   /* "Windows 10" -> "Windows 11" */
    }
    SET(si->os_name, "Windows");
    SET(si->os_detail, "%s%s%s (build %s)",
        prod[0] ? prod : "Windows",
        disp[0] ? " " : "", disp, build[0] ? build : "?");

    /* CPU */
    reg_str("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "ProcessorNameString", si->cpu_name, sizeof si->cpu_name);
    if (si->cpu_name[0]) {
        char *t = trim_ws(si->cpu_name);
        if (t != si->cpu_name) memmove(si->cpu_name, t, strlen(t) + 1);
    } else {
        SET(si->cpu_name, "Desconocido (%s)", si->arch);
    }

    si->cores_logical = (int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (si->cores_logical <= 0) si->cores_logical = (int)sys.dwNumberOfProcessors;

    /* RAM */
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof ms;
    if (GlobalMemoryStatusEx(&ms)) {
        si->ram_total = ms.ullTotalPhys;
        si->ram_avail = ms.ullAvailPhys;
    }

    /* Núcleos físicos y cachés */
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationAll, NULL, &len);
    if (len > 0) {
        BYTE *buf = (BYTE *)malloc(len);
        if (buf && GetLogicalProcessorInformationEx(
                RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf, &len)) {
            for (BYTE *p = buf; p < buf + len; ) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX e =
                    (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)p;
                if (e->Relationship == RelationProcessorCore) {
                    si->cores_physical++;
                } else if (e->Relationship == RelationCache) {
                    CACHE_RELATIONSHIP *c = &e->Cache;
                    uint64_t sz = c->CacheSize;
                    if (c->Level == 1 && c->Type == CacheData) {
                        if (sz > si->l1d) si->l1d = sz;
                    } else if (c->Level == 1 && c->Type == CacheInstruction) {
                        if (sz > si->l1i) si->l1i = sz;
                    } else if (c->Level == 2) {
                        if (sz > si->l2) si->l2 = sz;
                    } else if (c->Level == 3) {
                        if (sz > si->l3) si->l3 = sz;
                    }
                    if (!si->cache_line) si->cache_line = c->LineSize;
                }
                p += e->Size;
            }
        }
        free(buf);
    }
    if (si->cores_physical <= 0) si->cores_physical = si->cores_logical;
}

#endif

static void detect_system(SysInfo *si)
{
    memset(si, 0, sizeof *si);
#if defined(_WIN32)
    detect_windows(si);
#elif defined(__APPLE__)
    detect_macos(si);
#else
    detect_linux(si);
#endif
}

/* ------------------------------------------------------------------ */
/* Salida                                                              */
/* ------------------------------------------------------------------ */

static void print_text(const SysInfo *si)
{
    char a[32], b[32], c[32], d[32], e[32], f[32];
    fmt_bytes(si->ram_total, a, sizeof a);
    fmt_bytes(si->ram_avail, b, sizeof b);
    fmt_bytes(si->l1d, c, sizeof c);
    fmt_bytes(si->l1i, d, sizeof d);
    fmt_bytes(si->l2,  e, sizeof e);
    fmt_bytes(si->l3,  f, sizeof f);

    printf("=== Información del sistema ===\n");
    printf("SO            : %s\n", si->os_name);
    printf("Detalle SO    : %s\n", si->os_detail);
    printf("Arquitectura  : %s (%zu bits)\n", si->arch, sizeof(void *) * 8);
    printf("CPU           : %s\n", si->cpu_name);
    printf("Núcleos       : %d físicos, %d lógicos\n",
           si->cores_physical, si->cores_logical);
    printf("RAM total     : %s\n", a);
    printf("RAM disponible: %s\n", b);
    printf("Caché L1d     : %s\n", c);
    printf("Caché L1i     : %s\n", d);
    printf("Caché L2      : %s\n", e);
    printf("Caché L3      : %s\n", f);
    if (si->cache_line)
        printf("Línea de caché: %u B\n", si->cache_line);
}

static void json_str(const char *k, const char *v)
{
    printf("  \"%s\": \"", k);
    for (; *v; v++) {
        if (*v == '"' || *v == '\\') putchar('\\');
        if ((unsigned char)*v >= 0x20) putchar(*v);
    }
    printf("\",\n");
}

static void print_json(const SysInfo *si)
{
    printf("{\n");
    json_str("os", si->os_name);
    json_str("os_detail", si->os_detail);
    json_str("arch", si->arch);
    json_str("cpu", si->cpu_name);
    printf("  \"cores_physical\": %d,\n", si->cores_physical);
    printf("  \"cores_logical\": %d,\n", si->cores_logical);
    printf("  \"ram_total_bytes\": %llu,\n", (unsigned long long)si->ram_total);
    printf("  \"ram_avail_bytes\": %llu,\n", (unsigned long long)si->ram_avail);
    printf("  \"l1d_bytes\": %llu,\n", (unsigned long long)si->l1d);
    printf("  \"l1i_bytes\": %llu,\n", (unsigned long long)si->l1i);
    printf("  \"l2_bytes\": %llu,\n", (unsigned long long)si->l2);
    printf("  \"l3_bytes\": %llu,\n", (unsigned long long)si->l3);
    printf("  \"cache_line_bytes\": %u\n", si->cache_line);
    printf("}\n");
}
