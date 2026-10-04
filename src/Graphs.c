
#include <Graphs.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <random.h>
#include <hash32.h>



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
    //
    // 1. Se elimina de la cola el árbol Bi que contiene el valor mínimo
    // 2. Se le quita de la raíz, y lo que queda es una cola binomial B(i-1)
    // 3. Se suman ambas colas
    
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

 
uint32_t extractMin_Fibo32(BinomialTreeForest32 *dest, const double *values) {
    // 
    // 1. Sacar de la cola el árbol Bk que contiene el mínimo
    // 2. Eliminar raíz del árbol Bk que contiene el mínimo, quedando
    //    la lista de sus hijos
    // 3. Agregar los hijos a la lista de la cola original
    // 4. Convertir el bosque de árboles binomiales a bosque binomial
    // 4.1 Se crea arreglo de A log_2 n referencias
    // 4.2 se recorre la lista de la cola y se inserta Bk si A[k] está libre
    //     si no, se hace carry.
    
    // 1. extraer mínimo
    uint32_t z = dest->minNode;
 
    // Bosque binomial temporal. Comparte parents, children y deg con dest (se copian solo los
    // punteros y valores), pero su lista de raíces es de largo 32, con el mismo formato que 
    // una cola binomial
    uint32_t slots[32];
    for(uint32_t d = 0; d < 32; d++) slots[d] = FREE;
    BinomialTreeForest32 binomial = *dest;
    binomial.roots = slots;
    binomial.rootCount = 32;
    binomial.minNode = FREE;
 
    // Se inserta cada árbol de la lista (menos z) en el bosque binomial con carry.
    for(uint32_t i = 0; i < dest->rootCount; i++) {
        uint32_t r = dest->roots[i];
        if(r != z) carry(&binomial, values, r, dest->deg[r]);
    }
 
    // Los hijos de z quedan sin padre y se insertan igual. carry les corrige parents al
    // dejarlos como raíz o al enlazarlos bajo otro nodo.
    uint64_t childIndex = (uint64_t)z * 32; 
    for(uint32_t d = 0; d < dest->deg[z]; d++) {
        uint32_t c = dest->children[childIndex + d];
        carry(&binomial, values, c, dest->deg[c]);
    }
 
    // Volver al formato de Fibonacci, actualiza la cantidad de raíces almacenadas
    dest->rootCount = 0;
    for(uint32_t d = 0; d < 32; d++) {
        if(slots[d] != FREE) dest->roots[dest->rootCount++] = slots[d];
    }
    // el mínimo ya fue calculado por carry
    dest->minNode = binomial.minNode;
 
    // z queda sin hijos y se retorna
    dest->deg[z] = 0;
    return z;
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

void decreaseKey_Fibo32(BinomialTreeForest32 *dest, double *values, uint32_t x, double cost, uint8_t *flags) {
    values[x] = cost;
    uint32_t y = dest->parents[x];
    if( y != ROOT && values[x] < values[y]) {
        cut(dest, x, y, flags);
        cascadingCut(dest, y, flags);
    }
    if( dest->minNode == FREE || values[x] < values[dest->minNode]) {
        dest->minNode = x;
    }
}


MST32* Prim_Binomial32(const WGraph32* graph, uint32_t src, MST32* dest) {
    uint32_t n = graph->nodeCount;
    
    double* key = &(dest->key[0]);
    uint32_t* parent = &(dest->parent[0]);
    uint8_t* inMST = (uint8_t*)calloc(n, sizeof(uint8_t));
    if(!inMST) return NULL;
    
    for(uint32_t v = 0; v < n; v++) {
        key[v] = INFINITY;
        parent[v] = ROOT;
    }
    key[src] = 0.0;

    BinomialTreeForest32 * q = HEAP_BINOMIAL_QUEUE(n, key);
    if(!q) { free(inMST); return NULL; }

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


MST32* Prim_Fibo32(const WGraph32* graph, uint32_t src, MST32* dest) {
    uint32_t n = graph->nodeCount;
 
    double* key = &(dest->key[0]);
    uint32_t* parent = &(dest->parent[0]);
 
    // reserva para inMST y flags
    // 1 si está en el arbol 0 si no.
    uint8_t* inMST = (uint8_t*)calloc((size_t)n * 2, sizeof(uint8_t));
    if(!inMST) return NULL;
    uint8_t* flags = inMST + n;
 
    // inicializar key y parents
    for(uint32_t v = 0; v < n; v++) {
        key[v] = INFINITY;
        parent[v] = ROOT;
    }
    key[src] = 0.0;
 
    // cola de Fibonacci con los n nodos como raíces
    // minNode queda en src 0.0
    BinomialTreeForest32 * q = HEAP_FIBO_QUEUE(n, key);
    if(!q) { free(inMST); return NULL; }
 
    // se extrae un nodo por iteración
    for(uint32_t processed = 0; processed < n; processed++) {
        // nodo fuera del árbol con menor key
        uint32_t u = extractMin_Fibo32(q, key);
        // el resto no es alcanzable desde src (grafo no conexo)
        if(key[u] == INFINITY && u != src) break;
        // u entra al árbol
        inMST[u] = 1;
 
        // aristas de u (revisar documentación WGraph32)
        uint64_t start = graph->offsets[u];
        uint64_t end   = graph->offsets[u+1];
 
        for(uint64_t i = start; i < end; i++) { // for v in vecinos[u]
            uint32_t v = graph->edges[i];       // vecino
            // peso de la arista {u, v}
            double   w = (double)graph->weights[i]; 
            // solo se mejora a vecinos que siguen en la cola 
            // y con una arista más barata
            if(!inMST[v] && w < key[v]) {
                parent[v] = u;                  // v se conectaría al árbol mediante u
                // decreaseKey asigna key[v] = w y reordena la cola
                decreaseKey_Fibo32(q, key, v, w, flags);
            }
        }
    }
    free(inMST);
    free(q);
 
    dest->nodeCount = n;
    return dest;
}