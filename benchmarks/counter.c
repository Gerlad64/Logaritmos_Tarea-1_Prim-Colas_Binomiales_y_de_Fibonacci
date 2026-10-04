/**
 * counter.c  (benchmarks/counter.c)
 * Almacena y expone los contadores de llamadas y el tiempo acumulado en
 * decreaseKey. Los incrementos los hacen las macros COUNT_CALL / TIME_CALL de
 * counter.h dentro de Graphs.c, así que este archivo es independiente del
 * sistema operativo (Linux, WSL, macOS y Windows) (espero).
 */

#ifndef BENCH_COUNT
#error "counter.c debe compilarse con -DBENCH_COUNT"
#endif

#include "counter.h"

// Contadores y tiempos, inicializados en 0
unsigned long long call_counts[FUN_COUNT] = {0};
unsigned long long call_time_ns[FUN_COUNT] = {0};
int bench_timing_enabled = 0;

// Retorna el contador de una función específica
unsigned long long get_call_count(FuncID id) {
    if (id >= 0 && id < FUN_COUNT) {
        return call_counts[id];
    }
    return 0; // Identificador inválido
}

// Retorna el tiempo acumulado (ns) de una función medida con TIME_CALL
unsigned long long get_call_time_ns(FuncID id) {
    if (id >= 0 && id < FUN_COUNT) {
        return call_time_ns[id];
    }
    return 0;
}

// Reinicia un contador específico
void reset_call_count(FuncID id) {
    if (id >= 0 && id < FUN_COUNT) {
        call_counts[id] = 0;
        call_time_ns[id] = 0;
    }
}

// Utilidad: Reiniciar todos los contadores a la vez
void reset_all_call_counts(void) {
    for (int i = 0; i < FUN_COUNT; i++) {
        call_counts[i] = 0;
        call_time_ns[i] = 0;
    }
}

void set_dkey_timing(int on) {
    bench_timing_enabled = on ? 1 : 0;
}

// Mide cuánto agrega el cronómetro a una región vacía (par start/stop).
// Sirve para descontarlo: tiempo_real ≈ time_ns - llamadas * overhead.
double calibrate_timer_overhead_ns(void) {
    enum { N = 1000000 };
    int prev = bench_timing_enabled;
    unsigned long long before = call_time_ns[DKEY_BIN];

    bench_timing_enabled = 1;
    for (int k = 0; k < N; k++) {
        BenchTimer t = bench_timer_start(DKEY_BIN);
        bench_timer_stop(&t);
    }
    double ovh = (double)(call_time_ns[DKEY_BIN] - before) / N;

    call_time_ns[DKEY_BIN] = before;
    bench_timing_enabled = prev;
    return ovh;
}