#ifndef COUNTER_H
#define COUNTER_H

/*
 * counter.h
 *
 * Instrumentación para el benchmark. Solo está activa con -DBENCH_COUNT;
 * en cualquier otro build las macros desaparecen y no hay costo ni dependencia.
 *
 *   COUNT_CALL(id)  cuenta una llamada u operación estructural (swap, cut).
 *   TIME_CALL(id)   cuenta la llamada y, si el cronometraje está activo
 *                   (set_dkey_timing(1)), acumula el tiempo que la función
 *                   tarda en total, incluso con return tempranos
 *                   (usa __attribute__((cleanup)): GCC y Clang).
 *                   Va como PRIMERA línea de decreaseKey_*.
 */

// Definición de los identificadores
typedef enum {
    DKEY_BIN,
    DKEY_FIBO,
    SWP,
    CUT,
    FUN_COUNT
} FuncID;

// Firmas de las funciones que usarás en tu benchmark
unsigned long long get_call_count(FuncID id);
void reset_call_count(FuncID id);
void reset_all_call_counts(void);

// Tiempo acumulado (ns) dentro de las funciones medidas con TIME_CALL
unsigned long long get_call_time_ns(FuncID id);
// Activa/desactiva el cronometraje por llamada (desactivado por defecto)
void set_dkey_timing(int on);
// Costo medio (ns) que el cronómetro agrega a cada llamada medida
double calibrate_timer_overhead_ns(void);

#ifdef BENCH_COUNT
#include <time.h>

extern unsigned long long call_counts[FUN_COUNT];
extern unsigned long long call_time_ns[FUN_COUNT];
extern int bench_timing_enabled;

static inline unsigned long long bench_now_ns(void) {
    struct timespec ts;
#if defined(__APPLE__)
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);   /* resolución de ns (MONOTONIC es de µs en macOS) */
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    return (unsigned long long)ts.tv_sec * 1000000000ULL + (unsigned long long)ts.tv_nsec;
}

typedef struct {
    FuncID id;
    unsigned long long t0;   /* 0 = sin cronometrar */
} BenchTimer;

static inline BenchTimer bench_timer_start(FuncID id) {
    BenchTimer t = { id, bench_timing_enabled ? bench_now_ns() : 0ULL };
    return t;
}

static inline void bench_timer_stop(BenchTimer* t) {
    if (t->t0) call_time_ns[t->id] += bench_now_ns() - t->t0;
}

#define COUNT_CALL(id) (call_counts[(id)]++)
#define TIME_CALL(id) \
    COUNT_CALL(id); \
    BenchTimer bench_timer_ __attribute__((cleanup(bench_timer_stop))) = bench_timer_start(id)
#else
#define COUNT_CALL(id) ((void)0)
#define TIME_CALL(id)  ((void)0)
#endif

#endif // COUNTER_H