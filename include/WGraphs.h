

#pragma once

#include <hash32.h>

/**
 * @struct WGraph32
 * @brief Grafo no dirigido con pesos representado mediante arreglos.
 *
 * Almacena la topología de adyacencia y el peso de cada arista. No almacena los valores
 * asociados a cada nodo. Los identificadores de nodos son contiguos en el rango [0, nodeCount - 1].
 *
 * @details
 * Al ser un grafo no dirigido, cada arista {u, v} se almacena dos veces en @p edges:
 * una en la lista de adyacencia de @c u y otra en la de @c v. Por ende, el largo del
 * arreglo equivale al doble del número real de aristas no dirigidas.
 *
 * Los vecinos de un nodo @c u se encuentran en:
 * @code
 * uint32_t start = graph->offsets[u];
 * uint32_t end   = graph->offsets[u+1]; // edgeCount == offsets[nodeCount]
 * // Vecinos de u: edges[start ... end - 1]
 * // Grado de u:   end - start
 * @endcode
 * Donde el largo del arreglo @p edges es almacenado en el último elemento de @p offsets
 *
 * El peso de la arista {u, v} se obtiene consultando @p edges y @p weights:
 * @code
 * // vecinos de u
 * uint32_t i = graph->offsets[u];
 * uint32_t end   = graph->offsets[u+1];
 * // buscar a v entre los vecinos de u
 * for(; i < end; i++) {
 *     if( graph->edges[i] == v ) break;
 * }
 * // arista no encontrada
 * if( i == end ) return;
 * double w = graph->weights[i];
 * /// edges  : u1 u2 ...  v ...
 * ///          ↓  ↓  ...  ↓ ...
 * /// weights: w1 w2 ... wv ...
 * @endcode
 */
typedef struct {
    /** Número total de nodos en el grafo */
    uint32_t  nodeCount;
    /**  Arreglo con los destinos de las aristas */
    uint32_t* edges;
    /** Arreglo de tamaño @p nodeCount+1 con los índices para acceder a los vecinos de un nodo */
    uint64_t* offsets;
    /** Pesos de las aristas, en correspondencia 1 a 1 con el arreglo de aristas @p edges */
    double* weights;
} WGraph32;


#define STACK_WGRAPH32(N, E) \
    { \
        .nodeCount = (N), \
        .edges = (uint32_t[2*(E)]){0}, \
        .offsets = (uint64_t[(N)+1]){0}, \
        .weights = (double[2*(E)]){0} \
    }

static inline WGraph32* HEAP_WGRAPH32(uint32_t n, uint32_t e) {
    size_t off_size = ((size_t)n + 1) * sizeof(uint64_t);
    size_t w_size = (size_t)e * 2 * sizeof(double);
    size_t e_size = (size_t)e * 2 * sizeof(uint32_t);
    size_t total_size = sizeof(WGraph32) + off_size + w_size + e_size;

    void *ptr = calloc(1, total_size);
    if (!ptr) return NULL;

    WGraph32 *graph = (WGraph32*)ptr;
    char *data = (char*)ptr + sizeof(WGraph32);

    graph->offsets =(uint64_t*)data;
    data+= off_size;

    graph->weights = (double*)data;
    data+= w_size;

    graph->edges = (uint32_t*)data;
    graph->nodeCount = n;

    return graph;
}

static inline WGraph32* HEAP_WGRAPH32_IJ(uint8_t i, uint8_t j) {
    if (i > 31 || j > 58) return NULL;
    return HEAP_WGRAPH32((uint32_t)1 << i, (uint32_t)1 << j);
}

/**
 * @brief Inserta aristas en un grafo con memoria ya asignada y cantidad de nodos inicializada, pero sin aristas.
 */
void insertManyEmpty(uint32_t *restrict u, uint32_t *restrict v, double *weights, uint64_t edgeCount, WGraph32* dest);

/**
 * @brief Retorna un puntero con memoria auxiliar requerida por `randomWGraph32` alocada.
 * @details
 * Reserva memoria para ser usada en múltiples llamadas a randomWGraph32
 * 
 * @param max_j tamaño de j más grande a usar en múltiples llamadas de randomWGraph, 
 * donde 2^j es la cantidad de aristas
 * @return Puntero a memoria resevada, NULL si no se pudo reservar la memoria.
 */
static inline char* randomWGraph32Pool(uint8_t max_j) {
   uint64_t edgeCount = (uint64_t)1 << max_j;
   size_t edgeSize = 2 * edgeCount * sizeof(uint32_t); // se multiplica por 2: nodo origen y nodo destino
   size_t weightSize = edgeCount * sizeof(double);
   size_t hashSize = hash_size(edgeCount);
   size_t poolSize = edgeSize + weightSize + hashSize;
   return malloc(poolSize);
}

void randomWGraph32Seed(uint64_t seed);

/**
 * @brief Rellena @p dest (con memoria ya alocada para 2^i nodos y 2^j vertices) con un grafo aleatorio,
 * conexo, simple, y no dirigido.
 */
WGraph32* randomWGraph32(uint8_t i, uint8_t j, WGraph32* dest, char* pool);

/** @return 1 si todos los nodos son alcanzables desde el nodo 0, 0 si no. */
static int isConnected(const WGraph32* graph) {
    if(!graph) return 0;
    
    uint32_t nodeCount = graph->nodeCount;
    uint32_t* pendingNodes = (uint32_t*)malloc(nodeCount * sizeof(uint32_t));   /* pila del DFS */
    uint8_t*  visited      = (uint8_t*) calloc(nodeCount, sizeof(uint8_t));
    uint64_t  pendingCount = 0;
    uint32_t  reachedCount = 1;

    pendingNodes[pendingCount++] = 0;
    visited[0] = 1;
    while (pendingCount > 0) {
        uint32_t node = pendingNodes[--pendingCount];
        for (uint64_t pos = graph->offsets[node]; pos < graph->offsets[node + 1]; pos++) {
            uint32_t neighbor = graph->edges[pos];
            if (!visited[neighbor]) {
                visited[neighbor] = 1;
                reachedCount++;
                pendingNodes[pendingCount++] = neighbor;
            }
        }
    }
    free(pendingNodes); free(visited);
    return reachedCount == nodeCount;
}