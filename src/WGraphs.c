
#include <WGraphs.h>
#include <random.h>


void insertManyEmpty(uint32_t *restrict u, uint32_t *restrict v, double *weights, uint64_t edgeCount, WGraph32 *dest) {
    uint32_t V = dest->nodeCount;
    // inicializa en 0 arreglo de offsets
    memset(dest->offsets, 0, (V + 1) * sizeof(uint64_t));

    // Contar el grado de cada nodo, almacenando la cuenta en el arreglo de offsets
    // temporalmente (se hace dos veces, ya que una arista se almacena dos veces)
    for (uint64_t i = 0; i < edgeCount; i++) {
        dest->offsets[u[i] + 1]++;
        dest->offsets[v[i] + 1]++;
    }

    // Con los grados de cada nodo guardados en offsets, se puede determinar
    // donde empieza la lista de edges.
    for (uint32_t i = 0; i < V; i++) {
        dest->offsets[i + 1] += dest->offsets[i];
    }

    // Rellena edges y weights. Se usa offsets[x] como cursor:
    // cada vez que se agrega un vecino a la lista de x, el cursor avanza una posición.
    for (uint64_t i = 0; i < edgeCount; i++) {
        uint32_t a = u[i], b = v[i];
        double w = weights[i];

        uint64_t posInA = dest->offsets[a]++; // siguiente posición libre en la lista de a
        uint64_t posInB = dest->offsets[b]++; // siguiente posición libre en la lista de a

        dest->edges[posInA] = b;
        dest->weights[posInA] = w;
        dest->edges[posInB] = a;
        dest-> weights[posInB] = w;
    }

    for(uint32_t x = V; x > 0; x--) {
        dest->offsets[x] = dest->offsets[x-1];
    }
    dest->offsets[0] = 0;
}




WGraph32* randomWGraph32(uint8_t i, uint8_t j, WGraph32 *dest, char* pool) {
    uint64_t nodeCount = (uint64_t)1 << i; // 2^i vertices
    uint64_t edgeCount = (uint64_t)1 << j; // 2^j aristas

    char* ptr = pool;
    if( ptr == NULL) {
        ptr = randomWGraph32Pool(j);
        if( !ptr ) return NULL;
    }
    /** Alocación de variables auxiliares y tabla hash */
    size_t offset = 0;
    
    uint32_t* sourceNodes = (uint32_t*)ptr;
    offset += edgeCount * sizeof(uint32_t);
    
    uint32_t* targetNodes = (uint32_t*)(ptr + offset);
    offset += edgeCount * sizeof(uint32_t);

    double* edgeWeights = (double*)(ptr + offset);
    offset += edgeCount * sizeof(double);

    EdgeHash* edgeTable = hash_alloc(edgeCount, ptr + offset);

    /** Arbol Cobertor */
    uint64_t edgesGenerated = 0;
    for( uint32_t node = 1; node < nodeCount; node++) {
        uint32_t parent = randint(node);
        hash_try_insert(edgeTable, node, parent);
        sourceNodes[edgesGenerated] = node;
        targetNodes[edgesGenerated] = parent;
        edgeWeights[edgesGenerated] = randweight();
        edgesGenerated++;
    }
    /** Aristas restantes */
    while (edgesGenerated < edgeCount) {
        uint32_t nodeA = randint((uint32_t)nodeCount);
        uint32_t nodeB = randint((uint32_t)nodeCount - 1);
        if (nodeB >= nodeA) nodeB++;
        
        if (!hash_try_insert(edgeTable, nodeA, nodeB))
            continue;
        sourceNodes[edgesGenerated] = nodeA;
        targetNodes[edgesGenerated] = nodeB;
        edgeWeights[edgesGenerated] = randweight();
        edgesGenerated++;
    }
    dest->nodeCount = (uint32_t)nodeCount;
    insertManyEmpty(sourceNodes, targetNodes, edgeWeights, edgeCount, dest);
    
    if( pool == NULL ) free(ptr);
    
    return dest; 
}