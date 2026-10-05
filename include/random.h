
#pragma once

#include <stdint.h>
#include <pcg_basic.h>

/** Estado del generador. Contiene una semilla fija y una secuencia */
static pcg32_random_t randomState = PCG32_INITIALIZER;

/** Establece una semilla en el generador usando una misma secuencia */
static inline void setSeed(uint64_t seed) {
    pcg32_srandom_r(&randomState, seed, (uint64_t)67);
}

/**
 * @brief Entero aleatorio en el rango [0, n)
 * @param bound Límite superior exclusivo, en el rango [1, 2^32 - 1]
 */
static inline uint32_t randint(uint32_t bound) {
    return pcg32_boundedrand_r(&randomState, bound);
}


/**
 * @brief Peso aleatorio univorme en el rango (0, 1]
 * 
 * @details
 * Construye un double aleatorio con dos enteros aleatorios,
 * y combinándolos para obtener una mantisa de 53 bits
 */
static inline double randweight() {
    uint64_t upper27Bits = pcg32_random_r(&randomState) >> 5;
    uint64_t lower26Bits = pcg32_random_r(&randomState) >> 6;
    uint64_t rand53 = (upper27Bits << 26) | lower26Bits;
    constexpr double mantisa_len = (double)((uint64_t)1 << 53); // 2^53
    return (double)(rand53 + 1) / mantisa_len;
}