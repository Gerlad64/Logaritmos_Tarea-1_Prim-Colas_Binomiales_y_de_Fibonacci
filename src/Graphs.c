
#include <Graphs.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <random.h>
#include <hash32.h>

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

void carry(BinomialTreeForest32 *dest, const double* nodeValues, uint32_t carryNode, uint32_t degree) {
    double carryValue = nodeValues[carryNode];
    // se realiza el proceso de carry recorriendo las raíces
    for(uint32_t r = degree; r < dest->rootCount; r++) {
        uint32_t currentRoot = dest->roots[r];

        // Si el espacio r está libre
        // ahora es ocupado por carryNode
        if( currentRoot == FREE ) {
            // colocar carryNode en una raíz
            dest->roots[r] = carryNode;
            dest->parents[carryNode] = ROOT;

            if(dest->minNode == FREE || carryValue <=nodeValues[dest->minNode])
                dest->minNode = carryNode;

            break;
        }
        // Si el espacio r está ocupado
        // se realiza la 'suma' de arboles

        // se escoge como nuevo valor de carry el de menor valor
        if( carryValue > nodeValues[currentRoot]) { // nodeValues[currentRoot] es un valor ya consultado
            addChildren(dest, currentRoot, carryNode);
            carryNode = currentRoot;
            carryValue = nodeValues[currentRoot];
        }
        else {
            // carryNode es menor o igual: adopta a currentRoot
            // y este pierde su estatus de raíz
            addChildren(dest, carryNode, currentRoot);
        }
        // se desaloja roots[r]
        dest->roots[r] = FREE;
    }
}

BinomialTreeForest32* initBinomialQueue32(uint32_t nodeCount, const double* nodeValues, BinomialTreeForest32* dest) {
    dest->nodeCount = nodeCount;
    dest->rootCount = 32;
    dest->minNode = FREE;
    for(uint32_t i = 0; i < dest->rootCount; i++) dest->roots[i] = FREE;
    for(uint32_t i = 0; i < nodeCount; i++) {
        dest->deg[i] = 0;
        dest->parents[i] = ROOT;
    }
    
    for(uint32_t i = 0; i < nodeCount; i++) {
        carry(dest, nodeValues, i, 0);
    }
    return dest; 
}

BinomialTreeForest32* initFiboQueue32(uint32_t nodeCount, const double* nodeValues, BinomialTreeForest32* dest) {
    dest->nodeCount = nodeCount;
    dest->rootCount = nodeCount;
    uint32_t minNode = 0;
    for(uint32_t i = 0; i < nodeCount; i++) {
        dest->parents[i] = ROOT;
        dest->roots[i] = i;
        dest->deg[i] = 0;
        // TODO inicializar hijos si es que son necesarios
        if( nodeValues[minNode] > nodeValues[i]) minNode = i;
    }
    dest->minNode = minNode;
    return dest; 
}

uint32_t extractMin_Binomial32(BinomialTreeForest32 *dest, const double *values) {
    
    uint32_t minNode = dest->minNode;
    // maxDeg también indica su posición en roots
    // ya que el mínimo necesariamente pertenecía a la raíz
    uint32_t maxDeg  = dest->deg[minNode];
    // se libera la raíz
    dest->roots[maxDeg] = FREE;
    // inicio de la lista de hijos
    uint64_t childIndex = (uint64_t)minNode * 32;
    // se hace FREE para encontrar el nuevo min
    dest->minNode = FREE;
    // Se hace carry para añadir los arboles 
    // huerfanos (hijos de minNode extraido)
    for(uint32_t deg = 0; deg < maxDeg; deg++) {
        carry(dest, values, dest->children[childIndex++], deg);
    }
    // carry actualiza el mínimo hasta maxDeg
    // por lo que quedan raíces por revisar
    for(uint32_t r = maxDeg + 1; r < dest->rootCount; r++) {
        uint32_t currentRoot = dest->roots[r];
        if(currentRoot != FREE && (
            dest->minNode == FREE ||
            values[currentRoot] < values[dest->minNode]
        )) {
           dest->minNode = currentRoot; 
        }
        
    }
    // retorna el nodo extraido
    dest->deg[minNode] = 0;
    return minNode;
}

void decreaseKey_Binomial32(BinomialTreeForest32 *dest, double *values, uint32_t x, double cost) {
    values[x] = cost;
    uint32_t y = dest->parents[x];
    
    double temp;

    while( y != ROOT && values[x] < values[y] ) {
        // values[node] <-> values[nodeDown]
        /* 
        temp = values[y];
        values[y] = values[x];
        values[x] = temp;
        */
        swapParentWithChild(dest, y, x);
        //x = y;
        y = dest->parents[x];
    }
    if( values[x] < values[dest->minNode]) {
        dest->minNode = x;
    }
}


MST32* Prim_Binomial32(const WGraph32* graph, uint32_t src, MST32* dest) {
    uint32_t n = graph->nodeCount;
    
    double* key = &(dest->key[0]);
    uint32_t* parent = &(dest->parent[0]);
    uint8_t* inMST = (uint8_t*)calloc(n, sizeof(uint8_t));
    
    for(uint32_t v = 0; v < n; v++) {
        key[v] = INFINITY;
        parent[v] = ROOT;
    }
    key[src] = 0.0;

    BinomialTreeForest32 * q = HEAP_BINOMIAL_QUEUE(n, key);

    for(uint32_t processed = 0; processed < n; processed++) {
        
        uint32_t u = extractMin_Binomial32(q, key);
        if(key[u] == INFINITY && u != src) break;
        inMST[u] = 1;

        uint64_t start = graph->offsets[u];
        uint64_t end   = graph->offsets[u+1];

        for(uint64_t i = start; i < end; i++) {
            uint32_t v = graph->edges[i];
            double   w = (double)graph->weights[i];
            if(!inMST[v] && w < key[v]) {
                key[v] = w;
                parent[v] = u;
                decreaseKey_Binomial32(q, key, v, w);
            }
        }
    }
    free(inMST);
    free(q);
    
    dest->nodeCount = n;
    return dest;
}