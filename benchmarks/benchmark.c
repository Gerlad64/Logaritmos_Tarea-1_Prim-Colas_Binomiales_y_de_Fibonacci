#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <WGraphs.h>
#include <Graphs.h>
#include <time.h>
#include <sysinfo.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "counter.h"

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "El formato .npy escrito aqui asume little-endian"
#endif

#ifdef _WIN32
  #include <direct.h>
  #include <io.h>
  #define MKDIR(p)   _mkdir(p)
  #define DUP(fd)    _dup(fd)
  #define DUP2(a, b) _dup2(a, b)
  #define CLOSE(fd)  _close(fd)
  #define FILENO(f)  _fileno(f)
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
  #define MKDIR(p)   mkdir((p), 0755)
  #define DUP(fd)    dup(fd)
  #define DUP2(a, b) dup2(a, b)
  #define CLOSE(fd)  close(fd)
  #define FILENO(f)  fileno(f)
#endif

#define RESULTS_DIR "results"

#define REPS 10

#define MEASURE_SEC(out_sec, fun) \
    do { \
        struct timespec _t_ini, _t_fin; \
        clock_gettime(CLOCK_MONOTONIC, &_t_ini); \
        fun; \
        clock_gettime(CLOCK_MONOTONIC, &_t_fin); \
        (out_sec) = (double)(_t_fin.tv_sec - _t_ini.tv_sec) + \
                    ((double)(_t_fin.tv_nsec - _t_ini.tv_nsec) / 1e9); \
    } while (0)

typedef struct {
    uint16_t i;
    uint16_t j;
    uint32_t w_match[REPS]; // weights match
    double times_b[REPS];
    double times_f[REPS];
    uint64_t calls[REPS * FUN_COUNT];   // [rep][FuncID]: llamadas / operaciones
    uint64_t time_ns[REPS * FUN_COUNT]; // [rep][FuncID]: tiempo acumulado en decreaseKey (0 si no se midio)
    double timer_ovh_ns;                // sobrecosto medio del cronometro por llamada
} BenchmarkResults;

static double g_timer_ovh_ns = 0.0;

char* pool;
WGraph32 *graph;
MST32* mst;

void init_test_elements() {
    pool = randomWGraph32Pool(24);
    graph = HEAP_WGRAPH32_IJ(22, 24);
    mst = HEAP_MST32((uint32_t)1<<22);
}

BenchmarkResults runSeries(uint8_t i, uint8_t j) {
    BenchmarkResults b = { 
        .i = i,
		.j = j,
		.w_match = {0},
		.times_b = {0},
		.times_f = {0},
		.calls = {0},
		.time_ns = {0},
		.timer_ovh_ns = g_timer_ovh_ns
    };
    
    double total_bin = 0.0;
    double total_fibo = 0.0;
    double t, wbin, wfib;
    uint32_t wtotal = 0;
    printf("\nConfiguracion: i = %d (V = %u), j = %d (E = %u)\n", i, 1<<i, j, 1<<j);
    for(uint32_t r = 1; r <= REPS; r++) {
        randomWGraph32(i, j, graph, pool);
        // Binomial
        t = 0.0;
        MEASURE_SEC(t, Prim_Binomial32(graph, 67, mst));
        wbin = getWeight(mst);
        total_bin += t;
        b.times_b[r-1] = t;
        b.calls[(r-1) * FUN_COUNT + DKEY_BIN] = get_call_count(DKEY_BIN);
        b.calls[(r-1) * FUN_COUNT + SWP] = get_call_count(SWP);
        b.time_ns[(r-1) * FUN_COUNT + DKEY_BIN] = get_call_time_ns(DKEY_BIN);
        // Fibonacci
        t = 0.0;
        MEASURE_SEC(t, Prim_Fibo32(graph, 67, mst));
        wfib = getWeight(mst);
        total_fibo += t;
        b.times_f[r-1] = t;
        b.calls[(r-1) * FUN_COUNT + DKEY_FIBO] = get_call_count(DKEY_FIBO);
        b.calls[(r-1) * FUN_COUNT + CUT] = get_call_count(CUT);
        b.time_ns[(r-1) * FUN_COUNT + DKEY_FIBO] = get_call_time_ns(DKEY_FIBO);
        b.w_match[r-1] =  fabs(wfib - wbin) < 0.001 ? 1 : 0;
        wtotal += b.w_match[r-1];
        if(r>1) printf("\033[3A\r");
        printf(
            "\033[K -> Repeticion: %d/%d\n"
            "\033[K    Pesos: WBin: %.3f\n"
            "\033[K    Pesos: WFib: %.3f\n"
            "\033[K    Calzan: %d/%d ...",
            r, REPS, wbin, wfib, wtotal, REPS
        );
        fflush(stdout);
        reset_all_call_counts();
    }
    printf(
        "\n  -> Completado. Tiempo Promedio de Ejecucion:"\
        "\n   -> Binomial : %.4f segundos"\
        "\n   -> Fibonacci: %.4f segundos\n" 
        "\n   -> Total    : %.4f segundos\n", 
        total_bin / REPS,
        total_fibo / REPS,
        (total_bin+total_fibo) / REPS
    );
    
    return b;
}


/* ------------------------------------------------------------------ */
/* Reporte final                                                       */
/* ------------------------------------------------------------------ */

static void print_info(int target, const char* flags, const char* compilador, const SysInfo* si) {
    printf("==========================================\n");
    printf("Compilador : %s\n", compilador);
    printf("Flags      : %s\n", flags);
    printf("Target     : O%d\n", target);
    printf("==========================================\n");
    print_text(si);
}

static double avg_time(const double* v) {
    double s = 0.0;
    for (int k = 0; k < REPS; k++) s += v[k];
    return s / REPS;
}

static uint64_t avg_calls(const uint64_t* calls, int key) {
    uint64_t s = 0;
    for (int k = 0; k < REPS; k++) s += calls[k * FUN_COUNT + key];
    return (s + REPS / 2) / REPS;   /* promedio entero redondeado */
}

static uint32_t sum_match(const uint32_t* w) {
    uint32_t s = 0;
    for (int k = 0; k < REPS; k++) s += w[k];
    return s;
}

static void print_sep(void) {
    static const int w[] = {5, 2, 2, 9, 9, 12, 12, 5, 12, 12, 12, 12};
    putchar('+');
    for (size_t k = 0; k < sizeof w / sizeof *w; k++) {
        for (int m = 0; m < w[k] + 2; m++) putchar('-');
        putchar('+');
    }
    putchar('\n');
}

static void print_row(const char* serie, const BenchmarkResults* b) {
    printf("| %-5s | %2u | %2u | %9u | %9u | %12.4f | %12.4f | %2u/%-2d | %12llu | %12llu | %12llu | %12llu |\n",
           serie, (unsigned)b->i, (unsigned)b->j,
           1u << b->i, 1u << b->j,
           avg_time(b->times_b), avg_time(b->times_f),
           sum_match(b->w_match), REPS,
           (unsigned long long)avg_calls(b->calls, DKEY_BIN),
           (unsigned long long)avg_calls(b->calls, SWP),
           (unsigned long long)avg_calls(b->calls, DKEY_FIBO),
           (unsigned long long)avg_calls(b->calls, CUT));
}

static void print_results_table(const BenchmarkResults* A, const BenchmarkResults* B,
                                const BenchmarkResults* C, const BenchmarkResults* D, int n) {
    const char* names[4] = {"A", "B", "C", "D"};
    const BenchmarkResults* series[4] = {A, B, C, D};

    printf("\nRESULTADOS (promedio de %d repeticiones; Pesos = repeticiones con MST de igual peso;\n", REPS);
    printf("conteos = llamadas/operaciones promedio por repeticion)\n");
    print_sep();
    printf("| %-5s | %2s | %2s | %9s | %9s | %12s | %12s | %-5s | %12s | %12s | %12s | %12s |\n",
           "Serie", "i", "j", "V", "E", "Bin prom (s)", "Fib prom (s)", "Pesos",
           "DKey Bin", "Swaps Bin", "DKey Fib", "Cuts Fib");
    print_sep();
    for (int s = 0; s < 4; s++) {
        for (int k = 0; k < n; k++) print_row(names[s], &series[s][k]);
        print_sep();
    }
}


/* ------------------------------------------------------------------ */
/* Guardado de resultados (results/)                                   */
/* ------------------------------------------------------------------ */

/* Deja solo [A-Za-z0-9.+]; el resto pasa a '_' (sin repetidos ni en los bordes). */
static void sanitize(const char* in, char* out, size_t cap, size_t maxlen) {
    size_t n = 0;
    int last_us = 1;
    for (; *in && n < maxlen && n + 1 < cap; in++) {
        unsigned char c = (unsigned char)*in;
        if (isalnum(c) || c == '.' || c == '+') { out[n++] = (char)c; last_us = 0; }
        else if (!last_us)                      { out[n++] = '_';     last_us = 1; }
    }
    while (n > 0 && (out[n-1] == '_' || out[n-1] == '.')) n--;
    out[n] = '\0';
}

/* "Apple clang version 21.0.0 (...)" -> "apple-clang-21.0.0"; "gcc (Ubuntu ...) 13.3.0" -> "gcc-13.3.0" */
static void compiler_short(const char* s, char* out, size_t n) {
    const char* name = "cc";
    if (strstr(s, "clang"))                                  name = strstr(s, "Apple") ? "apple-clang" : "clang";
    else if (strstr(s, "gcc") || strstr(s, "GCC") || strstr(s, "g++")) name = "gcc";

    char ver[32] = "";
    for (const char* p = s; *p; p++) {
        if (isdigit((unsigned char)*p) && (p == s || !isalnum((unsigned char)p[-1]))) {
            size_t k = 0;
            while ((isdigit((unsigned char)p[k]) || p[k] == '.') && k < sizeof ver - 1) { ver[k] = p[k]; k++; }
            ver[k] = '\0';
            if (strchr(ver, '.')) break;   /* version con punto */
            ver[0] = '\0';                 /* p. ej. "gcc-14": seguir buscando */
        }
    }
    if (ver[0]) snprintf(out, n, "%s-%s", name, ver);
    else        snprintf(out, n, "%s", name);
}

/* Nombre base de los archivos: SO_arch_CPU_compilador_O<target>[_flags] */
static void build_tag(char* out, size_t n, const SysInfo* si, const char* comp_raw,
                      const char* flags, int target) {
    char os[32], arch[24], cpu[48], comp[48], fl[48];
    sanitize(si->os_name,  os,   sizeof os,   24);
    sanitize(si->arch,     arch, sizeof arch, 16);
    sanitize(si->cpu_name, cpu,  sizeof cpu,  40);
    compiler_short(comp_raw, comp, sizeof comp);
    sanitize(flags, fl, sizeof fl, 40);

    int w = snprintf(out, n, "%s_%s_%s_%s_O%d", os, arch, cpu, comp, target);
    if (w > 0 && (size_t)w < n && fl[0] && strcmp(flags, "Ninguna") != 0)
        snprintf(out + w, n - (size_t)w, "_%s", fl);
}

static int ensure_results_dir(void) {
    if (MKDIR(RESULTS_DIR) == 0) return 1;
    return errno == EEXIST;
}

/*
 * Escribe un archivo .npy (formato de NumPy 1.0) con un arreglo estructurado:
 * un registro por configuracion, en orden A0..A4, B0..B4, C0..C4, D0..D4.
 * np.load("results/<tag>.npy") lo lee directamente, sin definir dtypes a mano.
 * Los campos se escriben uno a uno (sin padding) y en little-endian.
 */
static void write_npy_header(FILE* f, size_t n_records) {
    char hdr[1024];
    int n = snprintf(hdr, sizeof hdr,
        "{'descr': [('serie', '|S1'), ('i', '<u2'), ('j', '<u2'), "
        "('w_match', '<u4', (%d,)), ('times_b', '<f8', (%d,)), ('times_f', '<f8', (%d,)), "
        "('calls', '<u8', (%d, %d)), ('time_ns', '<u8', (%d, %d)), ('timer_ovh_ns', '<f8')], "
        "'fortran_order': False, 'shape': (%zu,), }",
        REPS, REPS, REPS, REPS, FUN_COUNT, REPS, FUN_COUNT, n_records);

    size_t total = 10 + (size_t)n + 1;                 /* magic(8) + len(2) + header + '\n' */
    size_t pad = (64 - total % 64) % 64;
    uint16_t hlen = (uint16_t)((size_t)n + pad + 1);

    static const unsigned char magic[8] = {0x93, 'N', 'U', 'M', 'P', 'Y', 1, 0};
    unsigned char len_le[2] = { (unsigned char)(hlen & 0xFF), (unsigned char)(hlen >> 8) };
    fwrite(magic, 1, sizeof magic, f);
    fwrite(len_le, 1, 2, f);
    fwrite(hdr, 1, (size_t)n, f);
    for (size_t k = 0; k < pad; k++) fputc(' ', f);
    fputc('\n', f);
}

static int save_results_npy(const char* path, const BenchmarkResults* const series[4],
                            int n_series_done, int per_series) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    write_npy_header(f, (size_t)n_series_done * (size_t)per_series);
    for (int s = 0; s < n_series_done; s++) {
        for (int k = 0; k < per_series; k++) {
            const BenchmarkResults* b = &series[s][k];
            char serie = (char)('A' + s);
            fwrite(&serie, 1, 1, f);
            fwrite(&b->i, sizeof b->i, 1, f);
            fwrite(&b->j, sizeof b->j, 1, f);
            fwrite(b->w_match, sizeof b->w_match[0], REPS, f);
            fwrite(b->times_b, sizeof b->times_b[0], REPS, f);
            fwrite(b->times_f, sizeof b->times_f[0], REPS, f);
            fwrite(b->calls, sizeof b->calls[0], REPS * FUN_COUNT, f);
            fwrite(b->time_ns, sizeof b->time_ns[0], REPS * FUN_COUNT, f);
            fwrite(&b->timer_ovh_ns, sizeof b->timer_ovh_ns, 1, f);
        }
    }
    int ok = (fflush(f) == 0) && !ferror(f);
    fclose(f);
    return ok;
}

/* Guarda el .npy con lo completado hasta ahora (si el programa se cae, no se pierde todo). */
static void checkpoint(const char* path, const BenchmarkResults* const series[4], int done) {
    if (!save_results_npy(path, series, done, 5))
        fprintf(stderr, "\nAviso: no se pudo escribir %s\n", path);
}

/* Imprime el reporte final (info + tabla) a stdout; se reutiliza para el .txt */
static void print_report(int target, const char* flags, const char* compilador, const SysInfo* si,
                         const BenchmarkResults* A, const BenchmarkResults* B,
                         const BenchmarkResults* C, const BenchmarkResults* D) {
    print_info(target, flags, compilador, si);
    print_results_table(A, B, C, D, 5);
}

/* Ejecuta print_report con stdout redirigido al archivo: el .txt queda identico a la pantalla. */
static int save_report_txt(const char* path, int target, const char* flags, const char* compilador,
                           const SysInfo* si, const BenchmarkResults* A, const BenchmarkResults* B,
                           const BenchmarkResults* C, const BenchmarkResults* D) {
    FILE* f = fopen(path, "w");
    if (!f) return 0;
    fflush(stdout);
    int saved = DUP(FILENO(stdout));
    if (saved < 0) { fclose(f); return 0; }
    DUP2(FILENO(f), FILENO(stdout));
    print_report(target, flags, compilador, si, A, B, C, D);
    fflush(stdout);
    DUP2(saved, FILENO(stdout));
    CLOSE(saved);
    fclose(f);
    return 1;
}

int main(int argc, char* argv[]) {
    init_test_elements();
    int target = 0;
    char* flags_usadas = "Ninguna";
    char* version_compilador = "Desconocida";

    if (argc > 1) {
        target = atoi(argv[1]);
    }
    if (argc > 2 && argv[2][0] != '\0') {
        flags_usadas = argv[2];
    }
    if (argc > 3 && argv[3][0] != '\0') {
        version_compilador = argv[3]; // Aquí capturas el string completo
    }

    SysInfo si;
    detect_system(&si);
    print_info(target, flags_usadas, version_compilador, &si);

    /* Carpeta y nombres de salida; calibracion del cronometro de decreaseKey */
    char tag[256], path_npy[320], path_txt[320];
    int can_save = ensure_results_dir();
    if (!can_save) fprintf(stderr, "Aviso: no se pudo crear la carpeta %s/\n", RESULTS_DIR);
    build_tag(tag, sizeof tag, &si, version_compilador, flags_usadas, target);
    snprintf(path_npy, sizeof path_npy, RESULTS_DIR "/%s.npy", tag);
    snprintf(path_txt, sizeof path_txt, RESULTS_DIR "/%s.txt", tag);
    g_timer_ovh_ns = calibrate_timer_overhead_ns();
    set_dkey_timing(0);   /* series A y B: solo tiempo total, sin cronometrar cada llamada */


    /** ------------ SERIES ---------------- */
        
    uint32_t A[5] = { 20, 21, 22, 23, 24};
    uint32_t B[5] = { 18, 19, 20, 21, 22};
    uint32_t C[5] = { 18, 19, 20, 21, 22};
    uint32_t D[5] = { 14, 15, 16, 17, 18};
    

    BenchmarkResults Ar[5], Br[5], Cr[5], Dr[5];
    const BenchmarkResults* const all_series[4] = { Ar, Br, Cr, Dr };
    
    printf("----INICIANDO SERIE A-----\n");
    for(uint32_t j=0; j < 5; j++) {
        Ar[j] = runSeries(20, A[j]);
    }
    if (can_save) checkpoint(path_npy, all_series, 1);
    
    printf("----INICIANDO SERIE B-----\n");
    for(uint32_t j=0; j < 5; j++) {
        Br[j] = runSeries(B[j], 24);
    } 
    if (can_save) checkpoint(path_npy, all_series, 2);

    set_dkey_timing(1);   /* series C y D: tiempo acumulado en decreaseKey */
    printf("----INICIANDO SERIE C-----\n");
    for(uint32_t j=0; j < 5; j++) {
        Cr[j] = runSeries(18, C[j]);
    }
    if (can_save) checkpoint(path_npy, all_series, 3);
    printf("----INICIANDO SERIE D-----\n");
    for(uint32_t j=0; j < 5; j++) {
        Dr[j] = runSeries(D[j], 22);
    }
    set_dkey_timing(0);
    if (can_save) checkpoint(path_npy, all_series, 4);
    printf("\n\n");
    print_report(target, flags_usadas, version_compilador, &si, Ar, Br, Cr, Dr);

    if (can_save) {
        if (save_report_txt(path_txt, target, flags_usadas, version_compilador, &si, Ar, Br, Cr, Dr))
            printf("\nResultados guardados en:\n  %s\n  %s\n", path_npy, path_txt);
        else
            fprintf(stderr, "Aviso: no se pudo escribir %s\n", path_txt);
    }

    free(pool);
    free(graph);
    free(mst);
    return 0;
}