/**
 * TEST DE INTEGRIDAD DE WGraphs.
 * VERIFICA LA INTEGRIDAD DE LOS WGraphs PERO
 * EL PRINCIPAL OBJETIVO ES VERIFICAR LOS GRAFOS ALEATORIOS.
 */

#include <WGraphs.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

WGraph32 graph;
uint32_t edges[18] = {
    1, 2,    // nodo 0
    0, 4,    // nodo 1
    0, 3, 6, // nodo 2
    2, 5,    // nodo 3
    1,       // nodo 4
    3, 8, 9, // nodo 5
    2, 7,    // nodo 6
    6,       // nodo 7
    5,       // nodo 8
    5        // nodo 9
};
uint64_t offsets[11] = {
    0,
    2,
    4,
    7,
    9,
    10,
    13,
    15,
    16,
    17,
    18
};
double weights[18] = {
    0.1, 0.2,
    0.1, 0.3,
    0.2, 0.4, 0.5,
    0.4, 0.6, 
    0.3, 
    0.6, 0.7, 0.8,
    0.5, 0.9,
    0.9,
    0.7,
    0.8,
};


void init_test_graphs() {
    graph.nodeCount = 10;
    graph.edges = edges;
    graph.offsets = offsets;
    graph.weights = weights;
}

    
void test_insertManyEmpty() {
    printf("----TESTS insertManyEmpty\n");
    printf("---Grafo de 10 nodos y 9 aristas\n");
    

    WGraph32 gInsert = STACK_WGRAPH32(10, 9);
    
    uint32_t u[9] = {  0,  0,  1,  2,  2,  3,  5,  5,  6};
    uint32_t v[9] = {  1,  2,  4,  3,  6,  5,  8,  9,  7};
    double   w[9] = {0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9};
    insertManyEmpty(u, v, w, 9, &gInsert);
    
    printf("offsets calzan\n");
    for(uint32_t i = 0; i < 11; i++) {
        assert(offsets[i] == gInsert.offsets[i]);
    }
    printf("aristas calzan\n");
    for(uint64_t i = 0; i < 10; i++) {
        uint32_t n = offsets[i];
        uint32_t end = offsets[i+1];
        for( ; n < end; n++) {
            assert(graph.edges[n] == gInsert.edges[n]);
        }
    }
    printf("pesos calzan\n");
    for(uint64_t i = 0; i < 10; i++) {
        uint32_t n = offsets[i];
        uint32_t end = offsets[i+1];
        for( ; n < end; n++) {
            assert(fabs(graph.weights[n] - gInsert.weights[n]) < 0.001);
        }
    }
}

void test_isConnected() {
    printf("----TESTS isConnected\n");
    printf("---Grafo de 10 nodos y 9 aristas Conexo\n");
    // grafo conexo
    assert(isConnected(&graph) && "Debería ser Conexo");

    printf("---Grafo de 10 nodos y 7 aristas no Conexo\n");
    WGraph32 gNotConn = STACK_WGRAPH32(10, 7);
    // 
    // elimina {2, 6} y {3, 5} de graph
    uint32_t u[7] = {  0,  0,  1,  2,      5,  5,  6};
    uint32_t v[7] = {  1,  2,  4,  3,      8,  9,  7};
    double   w[7] = {0.1,0.2,0.3,0.4,0.6,0.8,0.9};
    
    insertManyEmpty(u, v, w, 7, &gNotConn);
    assert(!isConnected(&gNotConn) && "Debería ser No Conexo");
}


/**
 * @brief Aborta (assert) si @p graph no es un grafo válido con @p expectedEdges aristas no dirigidas.
 * @param checkSymmetry Si es 1, verifica que cada arista aparezca en ambas listas con el mismo peso.
 *        Cuesta O(suma de grado^2): úsalo solo con grafos pequeños.
 */
static void validateWGraph32(const WGraph32* graph, uint64_t expectedEdges, int checkSymmetry) {
    uint32_t nodeCount = graph->nodeCount;

    /* --- Estructura de offsets --- */
    assert(graph->offsets[0] == 0 && "offsets[0] debe ser 0");
    assert(graph->offsets[nodeCount] == 2 * expectedEdges && "offsets[nodeCount] debe ser 2*edgeCount");
    for (uint32_t u = 0; u < nodeCount; u++)
        assert(graph->offsets[u] <= graph->offsets[u + 1] && "offsets debe ser no decreciente");

    /* --- Contenido de las listas de adyacencia --- */
    /* lastOwner[w] = (último nodo u cuya lista contenía a w) + 1; detecta repetidos sin ordenar */
    uint32_t* lastOwner = (uint32_t*)calloc(nodeCount, sizeof(uint32_t));
    for (uint32_t u = 0; u < nodeCount; u++) {
        for (uint64_t pos = graph->offsets[u]; pos < graph->offsets[u + 1]; pos++) {
            uint32_t neighbor = graph->edges[pos];
            double   weight   = graph->weights[pos];

            assert(neighbor < nodeCount && "vecino fuera de rango");
            assert(neighbor != u && "lazo (arista de un nodo a si mismo)");
            assert(lastOwner[neighbor] != u + 1 && "arista repetida en una lista de adyacencia");
            lastOwner[neighbor] = u + 1;
            assert(weight > 0.0 && weight <= 1.0 && "peso fuera de (0, 1]");

            if (checkSymmetry) {          /* u debe estar en la lista de neighbor, con el mismo peso */
                int found = 0;
                for (uint64_t q = graph->offsets[neighbor]; q < graph->offsets[neighbor + 1]; q++)
                    if (graph->edges[q] == u && graph->weights[q] == weight) { found = 1; break; }
                assert(found && "arista sin su par simetrico (o con peso distinto)");
            }
        }
    }
    free(lastOwner);

    /* --- Conexidad --- */
    assert(isConnected(graph) && "el grafo no es conexo");
}

static void test_boundaryValidSizes(void) {
    /* Los extremos válidos: i=1,j=0 (grafo completo de 2 nodos) y j = 2i-2 máximo posible en el rango */
    static const int cases[][2] = { {1, 0}, {2, 2}, {2, 2}, {3, 3}, {3, 4}, {4, 6} };
    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        int i = cases[c][0], j = cases[c][1];
        WGraph32* graph = HEAP_WGRAPH32_IJ(i, j);
        assert(graph != NULL);
        assert(randomWGraph32(i, j, graph, NULL) == graph);
        validateWGraph32(graph, (uint64_t)1 << j, 1);
        free(graph);
    }
}

static void test_sweep(void) {
    for (int i = 2; i <= 10; i++)
        for (int j = i; j <= 2 * i - 2 && j <= 18; j++)
            for (uint64_t seed = 1; seed <= 5; seed++) {
                randomWGraph32Seed(seed);
                WGraph32* graph = HEAP_WGRAPH32_IJ(i, j);
                assert(graph != NULL);
                assert(randomWGraph32(i, j, graph, NULL) == graph);
                validateWGraph32(graph, (uint64_t)1 << j, /*checkSymmetry=*/ j <= 14);
                free(graph);
            }
}

static void test_reproducibility(void) {
    WGraph32 *g1 = HEAP_WGRAPH32_IJ(8, 10), *g2 = HEAP_WGRAPH32_IJ(8, 10), *g3 = HEAP_WGRAPH32_IJ(8, 10);
    randomWGraph32Seed(99);  randomWGraph32(8, 10, g1, NULL);
    randomWGraph32Seed(99);  randomWGraph32(8, 10, g2, NULL);
    randomWGraph32Seed(100); randomWGraph32(8, 10, g3, NULL);

    uint64_t storedEdges = g1->offsets[g1->nodeCount];
    assert(memcmp(g1->edges,   g2->edges,   storedEdges * sizeof(uint32_t)) == 0);   /* misma semilla: igual */
    assert(memcmp(g1->weights, g2->weights, storedEdges * sizeof(double))   == 0);
    assert(memcmp(g1->weights, g3->weights, storedEdges * sizeof(double))   != 0);   /* otra semilla: distinto */
    free(g1); free(g2); free(g3);
}

static void test_weightMean(void) {
    WGraph32* graph = HEAP_WGRAPH32_IJ(12, 20);
    randomWGraph32Seed(7);
    randomWGraph32(12, 20, graph, NULL);
    uint64_t storedEdges = graph->offsets[graph->nodeCount];
    double sum = 0;
    for (uint64_t p = 0; p < storedEdges; p++) sum += graph->weights[p];
    double mean = sum / (double)storedEdges;
    assert(mean > 0.498 && mean < 0.502 && "peso medio lejos de 0.5");
    free(graph);
}


void test_randomWGraph32() {
    printf("----TESTS randomWGraph32\n");
    printf("---Grafos aleatorios son Conexos\n");
    WGraph32 *g = HEAP_WGRAPH32_IJ(15, 20);
    g = randomWGraph32(15, 20, g, NULL);
    assert(isConnected(g));
    free(g);
    

    printf("BoundaryValidSizes\n");
    test_boundaryValidSizes();
    printf("sweep\n");
    test_sweep();
    printf("reproducibility\n");
    test_reproducibility();
    printf("weightMean\n");
    test_weightMean();
    
}

int main() {
    printf("--------TESTS WGraphs.h/WGraphs.c-------\n");
    init_test_graphs();
    test_insertManyEmpty();
    test_isConnected();
    test_randomWGraph32();
    
    printf("--------TODOS LOS TESTS PASARON-------\n");
    return 0;
}