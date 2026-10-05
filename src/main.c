/**
 * Aplicación de Prim_Binomial32 y Prim_Fibo32 a un problema pequeño:
 * conectar con fibra óptica los edificios de un campus usando el mínimo de cable.
 * 
 * Entre dos edificios cualesquiera puede haber o no una zanja posible, según si se puede excavar ahí. 
 * En el código, {0, 1, 120} significa que se puede conectar Biblioteca con Rectoría, y que el tramo mide 120 m. 
 * Esas 12 zanjas posibles son las aristas del grafo. 
 * 
 * El problema es que no hace falta excavar todas. Basta con que cada edificio quede conectado a la red, directa o indirectamente. 
 * Por ejemplo, si Biblioteca ya llega a Rectoría y Rectoría llega a Ingeniería, no necesitas además una zanja directa de 
 * Biblioteca a Ingeniería (200 m), porque ya están conectados por el camino anterior. Excavarla sería gastar cable de más.
 * 
 * Prim elige cuáles zanjas excavar para dejar todo conectado con el menor total de metros. En el ejemplo, eso son 6 zanjas y 690 m, 
 * en vez de las 12 y 1870 m de excavarlas todas.
 * 
 * Entonces:
 *  - Nodo   : un edificio.
 *  - Arista : una zanja posible entre dos edificios, su peso son los metros de cable.
 *  - MST    : la red que conecta todos los edificios con el menor largo total de cable.
 *
 * Todos los pesos son DISTINTOS, por lo que el MST es ÚNICO y se puede comparar
 * arista por arista el resultado de ambas colas de prioridad.
 *
  */
#include <Graphs.h>
#include <stdio.h>
#include <assert.h>


/** 
typedef enum {
    BIBLIOTECA, 
    RECTORIA,
    INGENIERIA,
    CASINO,
    GIMNASIO,
    LABORATORIO,
    DATACENTER,
    BUILDING_COUNT
} Buildings;
*/

constexpr uint32_t NODE_COUNT = 7;
constexpr uint64_t EDGE_COUNT = 12;

/** Nombre de cada nodo, el identificador del nodo es su posición en el arreglo */
static const char* buildingNames[NODE_COUNT] = {
    "Biblioteca",   // nodo 0
    "Rectoria",     // nodo 1
    "Ingenieria",   // nodo 2
    "Casino",       // nodo 3
    "Gimnasio",     // nodo 4
    "Laboratorio",  // nodo 5
    "DataCenter"    // nodo 6
};

/** Zanjas posibles: la arista i une sourceNodes[i] con targetNodes[i] y mide edgeWeights[i] metros */
static uint32_t sourceNodes[EDGE_COUNT] = {  0,   0,   0,   1,   1,   2,   2,   2,   3,   3,   4,   5 };
static uint32_t targetNodes[EDGE_COUNT] = {  1,   2,   3,   2,   5,   3,   4,   5,   4,   6,   6,   6 };
static double   edgeWeights[EDGE_COUNT] = { 120, 200, 150,  90, 230, 110, 180, 160, 140, 260, 100, 130 };

/**
 * @brief Imprime las aristas del MST con el nombre de los edificios y el cable total.
 * @param title Título de la tabla
 * @param mst   Resultado de Prim
 * @param src   Nodo raíz usado en Prim, no tiene padre
 */
static void printMST(const char* title, const MST32* mst, uint32_t src) {
    printf("\n%s (raíz = %s)\n", title, buildingNames[src]);
    printf("  %-12s -> %-12s %8s\n", "Desde", "Hasta", "Metros");
    for(uint32_t v = 0; v < mst->nodeCount; v++) {
        if(v == src) continue; // la raíz no tiene arista hacia un padre
        printf("  %-12s -> %-12s %8.0f\n", buildingNames[mst->parent[v]], buildingNames[v], mst->key[v]);
    }
    printf("  %-27s %8.0f\n", "TOTAL de cable", getWeight(mst));
}

int main() {
    /** Grafo */
    // en el stack, ya que es pequeño
    WGraph32 graph = STACK_WGRAPH32(NODE_COUNT, EDGE_COUNT);
    insertManyEmpty(sourceNodes, targetNodes, edgeWeights, EDGE_COUNT, &graph);

    // metros de cable si se excavan todas las zanjas posibles
    double totalWeight = 0.0;
    for(uint64_t i = 0; i < EDGE_COUNT; i++) totalWeight += edgeWeights[i];

    printf("=== Red de fibra óptica del campus ===\n");
    printf("%u edificios, %lu zanjas posibles, %.0f m si se excavan todas.\n",
           NODE_COUNT, (unsigned long)EDGE_COUNT, totalWeight);

    /** Prim con ambas colas de prioridad */
    uint32_t src = 0;
    MST32* binomial = HEAP_MST32(NODE_COUNT);
    MST32* fibo     = HEAP_MST32(NODE_COUNT);
    assert(binomial && fibo);

    Prim_Binomial32(&graph, src, binomial);
    Prim_Fibo32(&graph, src, fibo);

    printMST("Prim con cola BINOMIAL", binomial, src);
    printMST("Prim con cola de FIBONACCI", fibo, src);

    /** Comparación: el MST es único, por lo que parent y key deben ser iguales */
    int sameTree = 1;
    for(uint32_t v = 0; v < NODE_COUNT; v++) {
        if(binomial->parent[v] != fibo->parent[v]) sameTree = 0;
        if(binomial->key[v]    != fibo->key[v])    sameTree = 0;
    }
    printf("\nAmbas colas entregan el mismo árbol: %s\n", sameTree ? "SÍ" : "NO");
    assert(sameTree);

    double mstWeight = getWeight(binomial);
    printf("Ahorro: %.0f m de %.0f m (%.0f%%) con %u zanjas en vez de %lu.\n",
           totalWeight - mstWeight, totalWeight, 100.0 * (totalWeight - mstWeight) / totalWeight,
           NODE_COUNT - 1, (unsigned long)EDGE_COUNT);

    free(binomial);
    free(fibo);
    return 0;
}