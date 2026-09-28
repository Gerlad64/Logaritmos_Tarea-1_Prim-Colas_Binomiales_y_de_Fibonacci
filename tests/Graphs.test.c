#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <Graphs.h>
#include <time.h>
#include <time.h>
#include <assert.h> // assert(int expression)


void test_initBinomialQueue32() {
    
}

void test_initFiboQueue32() {


}

void test_extractMin_Binomial32() {
    
}

void test_extractMin_Fibo32() {
    
}

void test_decreaseKey_Binomial32() {
    
}

void test_decreaseKey_Fibo32() {
    
}


/** 
int main() {
    printf("--------TESTS Graphs.h/Graphs.c-------\n");
    
    test_initBinomialQueue32();
    test_initFiboQueue32();
    test_extractMin_Binomial32();
    test_extractMin_Fibo32();
    test_decreaseKey_Binomial32();
    test_decreaseKey_Fibo32();

    printf("--------TODOS LOS TESTS PASARON-------\n");
    return 0;
}
*/

//// ------------------------------------------------------------------
//// Funciones Auxiliares de Memoria y Generación
//// ------------------------------------------------------------------
//
///**
// * Crea un grafo completo aleatorio (todos conectados con todos)
// * Esto garantiza que el grafo sea conexo y siempre exista un MST.
// */
//WGraph32* createRandomCompleteGraph(uint32_t nodeCount) {
//    WGraph32* graph = (WGraph32*)malloc(sizeof(WGraph32));
//    graph->nodeCount = nodeCount;
//    
//    uint64_t edgeCount = (uint64_t)nodeCount * (nodeCount - 1); // Aristas dirigidas
//    graph->offsets = (uint64_t*)malloc((nodeCount + 1) * sizeof(uint64_t));
//    graph->edges = (uint32_t*)malloc(edgeCount * sizeof(uint32_t));
//    graph->weights = (double*)malloc(edgeCount * sizeof(double));
//
//    // Generar una matriz temporal para asegurar que el peso u->v sea igual a v->u
//    double** adjMatrix = (double**)malloc(nodeCount * sizeof(double*));
//    for (uint32_t i = 0; i < nodeCount; i++) {
//        adjMatrix[i] = (double*)malloc(nodeCount * sizeof(double));
//        for (uint32_t j = 0; j < i; j++) {
//            double w = (rand() % 100) + 1.0; // Pesos aleatorios entre 1 y 100
//            adjMatrix[i][j] = w;
//            adjMatrix[j][i] = w;
//        }
//    }
//
//    // Llenar formato de arreglos contiguos (similar a CSR)
//    uint64_t currentIndex = 0;
//    for (uint32_t u = 0; u < nodeCount; u++) {
//        graph->offsets[u] = currentIndex;
//        for (uint32_t v = 0; v < nodeCount; v++) {
//            if (u == v) continue;
//            graph->edges[currentIndex] = v;
//            graph->weights[currentIndex] = adjMatrix[u][v];
//            currentIndex++;
//        }
//    }
//    graph->offsets[nodeCount] = currentIndex;
//
//    // Liberar matriz temporal
//    for (uint32_t i = 0; i < nodeCount; i++) free(adjMatrix[i]);
//    free(adjMatrix);
//
//    return graph;
//}
//
//void freeGraph(WGraph32* graph) {
//    if (!graph) return;
//    free(graph->offsets);
//    free(graph->edges);
//    free(graph->weights);
//    free(graph);
//}
//
//MST32* createMST(uint32_t nodeCount) {
//    MST32* mst = (MST32*)malloc(sizeof(MST32));
//    mst->nodeCount = nodeCount;
//    mst->parent = (uint32_t*)malloc(nodeCount * sizeof(uint32_t));
//    mst->key = (double*)malloc(nodeCount * sizeof(double));
//    return mst;
//}
//
//void freeMST(MST32* mst) {
//    if (!mst) return;
//    free(mst->parent);
//    free(mst->key);
//    free(mst);
//}
//
//// ------------------------------------------------------------------
//// Función Main de Pruebas
//// ------------------------------------------------------------------
//int main() {
//    srand(time(NULL));
//
//    // Tamaños requeridos para las pruebas
//    uint32_t testSizes[] = {5, 10, 15, 20};
//    int numTests = sizeof(testSizes) / sizeof(testSizes[0]);
//
//    printf("Iniciando pruebas del Algoritmo de Prim_Binomial32_Binomial32...\n");
//    printf("==========================================\n");
//
//    for (int t = 0; t < numTests; t++) {
//        uint32_t V = testSizes[t];
//        uint32_t sourceNode = 0; 
//        
//        printf("\n[Test %d] Grafo de %u Nodos\n", t + 1, V);
//        
//        // 1. Crear el grafo y la estructura para almacenar el MST
//        WGraph32* graph = createRandomCompleteGraph(V);
//        MST32* mst = createMST(V);
//        
//        // 2. Ejecutar la función a testear
//        // (Asegúrate de que tu implementación devuelva el puntero a dest o maneje bien el retorno)
//        MST32* result = Prim_Binomial32_Binomial32(graph, sourceNode, mst);
//        
//        if (result != NULL) {
//            double totalMSTWeight = 0.0;
//            
//            // 3. Evaluar e ImPrim_Binomial32_Binomial32ir el resultado
//            // Sumamos los key (pesos) de todos los nodos excepto la raíz
//            for (uint32_t i = 0; i < V; i++) {
//                if (i != sourceNode) {
//                    totalMSTWeight += result->key[i];
//                }
//            }
//
//            printf(" -> Ejecucion exitosa.\n");
//            printf(" -> Peso total del MST: %.2f\n", totalMSTWeight);
//
//            // Si es un grafo pequeño (5 nodos), imPrim_Binomial32_Binomial32imos el árbol detallado
//            if (V == 5) {
//                printf(" -> Topologia del MST (Nodo: Padre [Peso]):\n");
//                for (uint32_t i = 0; i < V; i++) {
//                    if (i == sourceNode) {
//                        printf("    Nodo %u: RAIZ\n", i);
//                    } else {
//                        printf("    Nodo %u: %u [%.2f]\n", i, result->parent[i], result->key[i]);
//                    }
//                }
//            }
//        } else {
//            printf(" -> FALLO: La funcion Prim_Binomial32_Binomial32 devolvio NULL.\n");
//        }
//        
//        // 4. Liberar memoria del test actual
//        freeMST(mst);
//        freeGraph(graph);
//    }
//
//    printf("\n==========================================\n");
//    printf("Pruebas finalizadas.\n");
//
//    return 0;
//}


// ------------------------------------------------------------------
// Herramientas de Benchmarking y Generación
// ------------------------------------------------------------------

// Generador de 32-bits real, ya que RAND_MAX suele ser 32767 en Linux/Windows
uint32_t rand32() {
    uint32_t r = 0;
    for (int i = 0; i < 4; i++) {
        r = (r << 8) ^ (rand() & 0xFF);
    }
    return r;
}

// Obtiene el tiempo en segundos
double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/**
 * Crea un grafo disperso conexo.
 * V: cantidad de nodos.
 * E: cantidad de aristas no dirigidas.
 */
WGraph32* createLargeConnectedGraph(uint32_t V, uint32_t E) {
    if (E < V - 1) {
        printf("Error: E (%u) debe ser >= V-1 (%u) para ser conexo.\n", E, V - 1);
        exit(1);
    }

    // 1. Arreglos temporales para construir aristas antes del formato final (CSR)
    uint32_t* tmpU = (uint32_t*)malloc(E * sizeof(uint32_t));
    uint32_t* tmpV = (uint32_t*)malloc(E * sizeof(uint32_t));
    double*   tmpW = (double*)malloc(E * sizeof(double));
    uint32_t* degree = (uint32_t*)calloc(V, sizeof(uint32_t));

    uint32_t edgeIdx = 0;

    // 2. Garantizar que el grafo sea conexo creando un árbol aleatorio (V-1 aristas)
    for (uint32_t i = 1; i < V; i++) {
        uint32_t u = i;
        uint32_t v = rand32() % i; // Conectar nodo i con algún nodo ya en el árbol
        double w = (rand32() % 1000) + 1.0; 
        
        tmpU[edgeIdx] = u;
        tmpV[edgeIdx] = v;
        tmpW[edgeIdx] = w;
        
        degree[u]++;
        degree[v]++;
        edgeIdx++;
    }

    // 3. Rellenar las aristas restantes aleatoriamente
    while (edgeIdx < E) {
        uint32_t u = rand32() % V;
        uint32_t v = rand32() % V;
        if (u == v) continue; // Evitar self-loops por simplicidad

        double w = (rand32() % 1000) + 1.0;
        
        tmpU[edgeIdx] = u;
        tmpV[edgeIdx] = v;
        tmpW[edgeIdx] = w;
        
        degree[u]++;
        degree[v]++;
        edgeIdx++;
    }

    // 4. Construir la estructura WGraph32
    WGraph32* graph = (WGraph32*)malloc(sizeof(WGraph32));
    graph->nodeCount = V;
    graph->offsets = (uint64_t*)malloc((V + 1) * sizeof(uint64_t));
    
    // Al ser no dirigido, se guarda el doble de aristas
    uint64_t totalDirectedEdges = (uint64_t)E * 2; 
    graph->edges = (uint32_t*)malloc(totalDirectedEdges * sizeof(uint32_t));
    graph->weights = (double*)malloc(totalDirectedEdges * sizeof(double));

    // Calcular prefix sums para offsets
    uint64_t currentOffset = 0;
    for (uint32_t i = 0; i < V; i++) {
        graph->offsets[i] = currentOffset;
        currentOffset += degree[i];
    }
    graph->offsets[V] = currentOffset; // Debe coincidir con totalDirectedEdges

    // Copiar offsets temporales para insertar
    uint64_t* insertOffsets = (uint64_t*)malloc(V * sizeof(uint64_t));
    for (uint32_t i = 0; i < V; i++) {
        insertOffsets[i] = graph->offsets[i];
    }

    // Llenar edges y weights
    for (uint32_t i = 0; i < E; i++) {
        uint32_t u = tmpU[i];
        uint32_t v = tmpV[i];
        double w = tmpW[i];

        // u -> v
        uint64_t posU = insertOffsets[u]++;
        graph->edges[posU] = v;
        graph->weights[posU] = w;

        // v -> u
        uint64_t posV = insertOffsets[v]++;
        graph->edges[posV] = u;
        graph->weights[posV] = w;
    }

    // Limpieza
    free(tmpU); free(tmpV); free(tmpW);
    free(degree); free(insertOffsets);

    return graph;
}

void freeGraph(WGraph32* graph) {
    if (!graph) return;
    free(graph->offsets);
    free(graph->edges);
    free(graph->weights);
    free(graph);
}

MST32* createMST(uint32_t V) {
    MST32* mst = (MST32*)malloc(sizeof(MST32));
    mst->nodeCount = V;
    mst->parent = (uint32_t*)malloc(V * sizeof(uint32_t));
    mst->key = (double*)malloc(V * sizeof(double));
    return mst;
}

void freeMST(MST32* mst) {
    if (!mst) return;
    free(mst->parent);
    free(mst->key);
    free(mst);
}

// ------------------------------------------------------------------
// Ejecutor de Serie
// ------------------------------------------------------------------
void runSeriesTest(int i_pow, int j_pow, int repetitions) {
    uint32_t V = 1U << i_pow;
    uint32_t E = 1U << j_pow;

    printf("\nConfiguracion: i = %d (V = %u), j = %d (E = %u)\n", i_pow, V, j_pow, E);
    
    double total_time = 0.0;

    for (int r = 1; r <= repetitions; r++) {
        // ImPrim_Binomial32ir progreso en misma linea
        printf("\r  -> Generando y probando repeticion %d/%d ...", r, repetitions);
        fflush(stdout);

        WGraph32* graph = createLargeConnectedGraph(V, E);
        MST32* dest = createMST(V);

        double t_start = get_time_sec();
        
        MST32* result = Prim_Binomial32(graph, 0, dest);
        
        double t_end = get_time_sec();
        double elapsed = t_end - t_start;

        if (result == NULL) {
            printf("\nERROR: Prim_Binomial32 devolvió NULL en la repeticion %d.\n", r);
            exit(1);
        }

        total_time += elapsed;

        freeGraph(graph);
        freeMST(dest);
    }

    double avg_time = total_time / repetitions;
    printf("\r  -> Completado. Tiempo Promedio de Ejecucion: %.4f segundos\n", avg_time);
}

// ------------------------------------------------------------------
// Main
// ------------------------------------------------------------------
int main() {
    srand((unsigned int)time(NULL));

    printf("=========================================================\n");
    printf("   BENCHMARKING MASIVO - ALGORITMO DE Prim_Binomial32 (V=2^i, E=2^j)\n");
    printf("=========================================================\n");

    int repetitions = 10;

    // SERIE A: i fijo (20), j varia
    printf("\n--- INICIANDO SERIE A (V fijo, i=20) ---\n");
    int j_serieA[] = {20, 21, 22, 23, 24};
    for (int k = 0; k < 5; k++) {
        runSeriesTest(20, j_serieA[k], repetitions);
    }

    // SERIE B: j fijo (24), i varia
    printf("\n--- INICIANDO SERIE B (E fijo, j=24) ---\n");
    int i_serieB[] = {18, 19, 20, 21, 22};
    for (int k = 0; k < 5; k++) {
        runSeriesTest(i_serieB[k], 24, repetitions);
    }

    printf("\n=========================================================\n");
    printf("Pruebas finalizadas.\n");

    return 0;
}