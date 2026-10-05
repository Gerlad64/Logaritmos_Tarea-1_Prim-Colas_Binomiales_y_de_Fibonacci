
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief: función de hash de 32 bits, donde cada entrada da una salida distinta.
 * Sacada de `https://github.com/skeeto/hash-prospector` (dominio público).
 */
static inline uint32_t lowbias32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

/**
 * @brief estructura auxiliar para manejar el hash
 * 
 */
typedef struct {
    /** tamaño de la tabla, usado como mascara para operaciones con bits
     * su valor es 2^exp - 1
      */
    uint64_t mask;
    /** Tabla para almacenar valores del hash y consultarlos en O(1) */
    uint64_t slots[];
} EdgeHash;

/**
 * @brief Dada una cantidad máxima de aristas a usar, devuelve el tamaño en memoria del hash.
 * @param maxEdges La cantidad máxima de aristas a usar en el hash.
 * @returns tamaño de la tabla hash, esto es, tamaño de la estructura EdgeHash más el espacio de la tabla
 */
static inline size_t hash_size(uint64_t maxEdges) {
    uint64_t tableSize = 1;
    while( tableSize < 2 * maxEdges ) tableSize <<= 1;
    return sizeof(EdgeHash) + tableSize * sizeof(uint64_t);
}

/**
 * @brief Inicializa una tabla hash usando una pool de memoria existente o
 * reservándo la memoria necesaria en el HEAP.
 */
static inline EdgeHash* hash_alloc(uint64_t maxEdges, char* pool) {
    uint64_t tableSize = 1;
    
    while( tableSize < 2 * maxEdges ) 
        tableSize <<= 1;
    
    size_t totalSize = sizeof(EdgeHash) + tableSize * sizeof(uint64_t);
    EdgeHash* table = NULL;
    
    if(pool == NULL) // no se porporcionó una pool
        table = (EdgeHash*)calloc(1, totalSize);
    else {
        memset(pool, 0, totalSize);
        table = (EdgeHash*)pool;
    }
    
    if(!table) return NULL;

    table->mask = tableSize - 1;
    return table;
}


/**
 * @brief Intenta registrar la arista no dirigida {nodeA, nodeB} en la tabla.
 * 
 * @details
 * Cada arista genera la siguiente clave `(<nodo-menor> << 32) | <nodo-mayor>`
 * 
 * @param table Tabla creada con hash_aloc
 * @param nodeA Un extremo de la arista
 * @param nodeB Otro extremo de la arista, **distinto** de @p nodeA
 * @return 1 si la arista era nueva y se insertó, 0 si ya estaba en la tabla (no se inserta).
 */
static inline int hash_try_insert(EdgeHash* table, uint32_t nodeA, uint32_t nodeB) {
    uint32_t smallerNode = (nodeA < nodeB) ? nodeA : nodeB;
    uint32_t largerNode  = (nodeA < nodeB) ? nodeB : nodeA;
    // llave única de {nodoA, nodoB}
    uint64_t edgeKey = ((uint64_t)smallerNode << 32) | largerNode;
    // posición en la tabla ajustado al tamaño
    uint64_t slot = lowbias32(smallerNode ^ lowbias32(largerNode)) & table->mask;
    while (table->slots[slot] != 0) {
        // llave ya existía en la table
        if (table->slots[slot] == edgeKey) return 0;
        // encontrar slot libre donde insertar
        slot = (slot + 1) & table->mask;
    }
    table->slots[slot] = edgeKey;
    return 1;
}
