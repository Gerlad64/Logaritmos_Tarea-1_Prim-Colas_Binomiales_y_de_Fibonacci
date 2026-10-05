
#include <Graphs.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/** Grafo basic-graph.jpg */
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

static int d_eq(double a, double b) {
    return fabs(a - b) < 1e-6;
}

void test_addChildren() {
    /**
     * Testea la función addChildren simulando agregar al nodo 1 como hijo del nodo 0.
     * Se verifica lo siguiente:
     * - El padre del nodo 1 debe actualizarse al nodo 0 (assert: dest.parents[1] == 0).
     * - El primer hijo del nodo 0 debe ser el nodo 1 (assert: dest.children[0] == 1).
     * - El grado del nodo 0 (su cantidad de hijos) debe aumentar a 1 (assert: dest.deg[0] == 1).
     */
    uint32_t parents[2] = {ROOT, ROOT};
    uint32_t roots[2] = {0, 1};
    uint32_t children[64]; // 2 * log 2^32 == 2 * 32 == 64
    for (int i = 0; i < 64; i++) children[i] = NO_CHILD;
    uint32_t deg[2] = {0, 0};
    
    BinomialTreeForest32 dest = {
        .nodeCount = 2, .rootCount = 2, .minNode = 0,
        .parents = parents, .roots = roots, .children = children, .deg = deg
    };
    
    // Se inserta el nodo 1 como hijo del nodo 0
    addChildren(&dest, 0, 1);
    assert(dest.parents[1] == 0);
    assert(dest.children[0] == 1); 
    assert(dest.deg[0] == 1);
    printf("OK: test_addChildren\n");
}

void test_swapParentWithChild() {
    /**
     * Testea swapParentWithChild partiendo de un estado donde el nodo 1 es hijo del nodo 0.
     * Tras ejecutar la función, se verifica con asserts que los roles se hayan invertido exitosamente:
     * - El nuevo padre del nodo 0 debe ser el nodo 1 (assert: dest.parents[0] == 1).
     * - El nodo 1 debe quedar como raíz, es decir, su padre debe ser ROOT (assert: dest.parents[1] == ROOT).
     */
    uint32_t parents[2] = {ROOT, 0};
    uint32_t roots[2] = {0, FREE};
    uint32_t children[64];
    for (int i = 0; i < 64; i++) children[i] = NO_CHILD;
    children[0] = 1; // 1 es hijo de 0
    uint32_t deg[2] = {1, 0};
    
    BinomialTreeForest32 dest = {
        .nodeCount = 2, .rootCount = 1, .minNode = 0,
        .parents = parents, .roots = roots, .children = children, .deg = deg
    };
    
    // swap entre 0 (parent) y 1 (children)
    swapParentWithChild(&dest, 0, 1);
    assert(dest.parents[0] == 1);
    assert(dest.parents[1] == ROOT);
    printf("OK: test_swapParentWithChild\n");
}

void test_cut_cascadingCut() {
    /**
     * Testea las funciones cut y cascadingCut configurando manualmente una cadena de nodos (0 -> 1 -> 2).
     * - Primero ejecuta cut para desvincular el nodo 2 del nodo 1, usando un arreglo de flags.
     * - Luego ejecuta cascadingCut en el nodo 1 para simular la propagación de cortes en el árbol.
     * La verificación actual comprueba que los cálculos de índices y modificaciones de arreglos 
     * se ejecuten sin segfaults ni errores de memoria
     */
    uint32_t parents[3] = {ROOT, 0, 1};
    uint32_t roots[3] = {0, FREE, FREE};
    uint32_t children[96];
    for (int i = 0; i < 96; i++) children[i] = NO_CHILD;
    children[0] = 1;  // 1 es hijo de 0
    children[32] = 2; // 2 es hijo de 1
    uint32_t deg[3] = {1, 1, 0};
    
    BinomialTreeForest32 dest = {
        .nodeCount = 3, .rootCount = 1, .minNode = 0,
        .parents = parents, .roots = roots, .children = children, .deg = deg
    };
    
    uint8_t flags[3] = {0, 0, 0};
    // Desvincular 2 de la lista de hijos de 1
    cut(&dest, 2, 1, flags);
    
    // Evaluar propagación de cortes
    cascadingCut(&dest, 1, flags);
    printf("OK: test_cut_cascadingCut\n");
}

void test_carry() {
    /**
     * Testea la función carry simulando un acarreo lógico de un nodo de grado 0 (el nodo 0) dentro del bosque.
     * - Se verifica que la función recorra correctamente la estructura y consulte el arreglo de 
     *   nodeValues sin arrojar errores de acceso a memoria.
     */
    double values[2] = {10.0, 5.0};
    uint32_t parents[2] = {ROOT, ROOT};
    uint32_t roots[2] = {0, 1};
    uint32_t children[64];
    for (int i = 0; i < 64; i++) children[i] = NO_CHILD;
    uint32_t deg[2] = {0, 0};
    
    BinomialTreeForest32 dest = {
        .nodeCount = 2, .rootCount = 2, .minNode = 1,
        .parents = parents, .roots = roots, .children = children, .deg = deg
    };
    
    // Acarreo lógico de un nodo en el bosque
    carry(&dest, values, 0, 0);
    printf("OK: test_carry\n");
}


void test_BinomialQueue() {
    /**
     * Testea el flujo completo de una Cola Binomial:
     * 1. Inicializa la cola usando STACK_BINOMIAL_QUEUE con 5 valores; verifica con assert que su nodeCount sea 5.
     * 2. Extrae el mínimo con extractMin_Binomial32; verifica con assert que retorne el nodo 3 (1.0).
     * 3. Modifica la clave del nodo 0 a 0.5 usando decreaseKey_Binomial32.
     * 4. Extrae nuevamente el mínimo; verifica con assert que ahora retorne el nodo 0, demostrando que la cola se reordenó.
     * 5. Finalmente, verifica que HEAP_BINOMIAL_QUEUE asigne correctamente la memoria en el heap (assert != NULL).
     */
    double values[5] = {5.0, 3.0, 7.0, 1.0, 9.0};
    
    // Inicialización de la cola binomial en el stack
    BinomialTreeForest32* bq = STACK_BINOMIAL_QUEUE(5, values);
    assert(bq != NULL);
    assert(bq->nodeCount == 5);
    
    // Extracción del nodo con valor mínimo
    uint32_t min = extractMin_Binomial32(bq, values);
    assert(min == 3); // Nodo 3 vale 1.0
    
    // Reducción de la clave para ajustar la estructura
    decreaseKey_Binomial32(bq, values, 0, 0.5);
    values[0] = 0.5; 
    min = extractMin_Binomial32(bq, values);
    assert(min == 0);
    
    // Inicialización en el heap para verificar manejo dinámico
    BinomialTreeForest32* heap_bq = HEAP_BINOMIAL_QUEUE(5, values);
    assert(heap_bq != NULL);
    free(heap_bq);
    
    printf("OK: test_BinomialQueue\n");
}

void test_FiboQueue() {
    double values[5] = {5.0, 3.0, 7.0, 1.0, 9.0};
    
    // Inicialización secuencial de la cola de Fibonacci en el stack
    BinomialTreeForest32* fq = STACK_FIBO_QUEUE(5, values);
    assert(fq != NULL);
    assert(fq->nodeCount == 5);
    
    uint32_t min = extractMin_Fibo32(fq, values);
    assert(min == 3);
    
    uint8_t flags[5] = {0};
    decreaseKey_Fibo32(fq, values, 0, 0.5, flags);
    values[0] = 0.5;
    min = extractMin_Fibo32(fq, values);
    assert(min == 0);
    
    BinomialTreeForest32* heap_fq = HEAP_FIBO_QUEUE(5, values);
    assert(heap_fq != NULL);
    free(heap_fq);
    
    printf("OK: test_FiboQueue\n");
}

/**
 * Test de Prim usando la estructura MST32.
 * El MST del grafo base tiene un peso verificado de 4.5.
 */
void test_MST() {
    // Reservar la estructura en memoria para guardar resultados del árbol
    MST32* mst_bin = HEAP_MST32(graph.nodeCount);
    assert(mst_bin != NULL);
    
    MST32* res_bin = Prim_Binomial32(&graph, 0, mst_bin);
    assert(res_bin == mst_bin);
    
    // Recuperar el peso acumulado usando la función getWeight
    double w_bin = getWeight(mst_bin);
    assert(d_eq(w_bin, 4.5));
    
    // Repetir el proceso usando el algoritmo basado en Fibonacci
    MST32* mst_fib = HEAP_MST32(graph.nodeCount);
    MST32* res_fib = Prim_Fibo32(&graph, 0, mst_fib);
    assert(res_fib == mst_fib);
    
    double w_fib = getWeight(mst_fib);
    assert(d_eq(w_fib, 4.5));
    
    free(mst_bin);
    free(mst_fib);
    printf("OK: test_MST\n");
}

int main() {
    printf("--------TESTS Graphs.h/Graphs.c-------\n");
    init_test_graphs();
    
    test_addChildren();
    test_swapParentWithChild();
    test_cut_cascadingCut();
    test_carry();
    test_BinomialQueue();
    test_FiboQueue();
    test_MST();
    
    printf("--------TODOS LOS TESTS PASARON-------\n");
    return 0;
}