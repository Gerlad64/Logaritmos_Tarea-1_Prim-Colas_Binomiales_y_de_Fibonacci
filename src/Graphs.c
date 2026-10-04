
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

// Tamaño de la tabla de grados usada al consolidar. Con ids de 32 bits el grado máximo de un
// árbol de una cola de Fibonacci es log_phi(2^32) ~ 46, así que 64 sobra.
#define FIBO_MAX_DEGREE 64
 
/**
 * Inserta la raíz @p w en la tabla de consolidación @p table (table[d] = raíz de grado d o FREE).
 * Si ya hay una raíz del mismo grado, las enlaza (la mayor pasa a ser hija de la menor) y repite
 * con el árbol resultante, que tiene un grado más.
 */
static inline void fiboLinkInsert(BinomialTreeForest32 *dest, const double *values,
                                  uint32_t *table, uint32_t w, uint8_t *flags) {
    // grado actual del árbol cuya raíz es w
    uint32_t d = dest->deg[w];
    // mientras exista otra raíz del mismo grado, hay que enlazar
    while(table[d] != FREE) {
        // y es la otra raíz de grado d
        uint32_t y = table[d];
        // w debe quedar como la raíz de menor valor (en empate se mantiene w)
        if(values[y] < values[w]) {
            uint32_t tmp = w;
            w = y;
            y = tmp;
        }
        // y pasa a ser hijo de w: parents[y] = w, children de w += y, deg[w]++
        addChildren(dest, w, y);
        // un nodo que pasa a ser hijo queda desmarcado (como en CLRS)
        flags[y] = 0;
        // el grado d queda libre: su árbol se acaba de fusionar
        table[d] = FREE;
        // el árbol resultante tiene un grado más (deg[w] ya fue incrementado por addChildren)
        d = dest->deg[w];
    }
    // no hay colisión: w ocupa el espacio de su grado
    table[d] = w;
}
 
uint32_t extractMin_Fibo32(BinomialTreeForest32 *dest, const double *values, uint8_t *flags) {
    // 1. nodo mínimo a extraer
    uint32_t z = dest->minNode;
 
    // 2. tabla de consolidación: table[d] = raíz de grado d (o FREE), inicialmente vacía
    uint32_t table[FIBO_MAX_DEGREE];
    for(uint32_t d = 0; d < FIBO_MAX_DEGREE; d++) table[d] = FREE;
 
    // 3. consolidar todas las raíces actuales excepto z (z es el que se extrae)
    //    roots[0..rootCount-1] es una lista densa; se lee completa antes de reescribirla
    uint32_t oldRootCount = dest->rootCount;
    for(uint32_t i = 0; i < oldRootCount; i++) {
        uint32_t r = dest->roots[i];
        if(r == z) continue;                         // z sale de la cola
        fiboLinkInsert(dest, values, table, r, flags);
    }
 
    // 4. los hijos de z pasan a ser raíces: se consolidan igual que las demás
    uint64_t childIndex = (uint64_t)z * 32;          // inicio de la lista de hijos de z
    uint32_t zDeg = dest->deg[z];                    // cantidad de hijos de z
    for(uint32_t d = 0; d < zDeg; d++) {
        uint32_t c = dest->children[childIndex + d]; // d-ésimo hijo de z
        dest->parents[c] = ROOT;                     // ya no tiene padre
        flags[c] = 0;                                // las raíces quedan desmarcadas
        fiboLinkInsert(dest, values, table, c, flags);
    }
 
    // 5. reconstruir la lista de raíces desde la tabla y encontrar el nuevo mínimo
    dest->rootCount = 0;
    dest->minNode = FREE;
    for(uint32_t d = 0; d < FIBO_MAX_DEGREE; d++) {
        uint32_t r = table[d];
        if(r == FREE) continue;                      // no hay raíz de este grado
        dest->roots[dest->rootCount++] = r;          // se agrega a la lista densa de raíces
        if(dest->minNode == FREE || values[r] < values[dest->minNode])
            dest->minNode = r;                       // candidato a nuevo mínimo
    }
 
    // 6. z queda como nodo suelto (sin hijos ni padre) y se retorna
    dest->deg[z] = 0;
    dest->parents[z] = ROOT;
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


MST32* Prim_Fibo32(const WGraph32* graph, uint32_t src, MST32* dest) {
    // cantidad de nodos del grafo
    uint32_t n = graph->nodeCount;
 
    // claves (key[v] = peso de la arista más barata conocida hacia v) y padres, viven en el MST de salida
    double* key = &(dest->key[0]);
    uint32_t* parent = &(dest->parent[0]);
 
    // una sola reserva (en cero) para dos arreglos de n bytes:
    // inMST[v] = 1 si v ya está en el árbol; flags[v] = marca de la cola de Fibonacci
    uint8_t* inMST = (uint8_t*)calloc((size_t)n * 2, sizeof(uint8_t));
    if(!inMST) return NULL;
    uint8_t* flags = inMST + n;
 
    // inicialmente ningún nodo es alcanzable salvo src
    for(uint32_t v = 0; v < n; v++) {
        key[v] = INFINITY;
        parent[v] = ROOT;
    }
    key[src] = 0.0;
 
    // cola de Fibonacci con los n nodos como raíces; minNode queda en src (key 0)
    BinomialTreeForest32 * q = HEAP_FIBO_QUEUE(n, key);
    if(!q) { free(inMST); return NULL; }
 
    // se extrae un nodo por iteración
    for(uint32_t processed = 0; processed < n; processed++) {
        // nodo fuera del árbol con menor key
        uint32_t u = extractMin_Fibo32(q, key, flags);
        // key infinita: el resto no es alcanzable desde src (grafo no conexo)
        if(key[u] == INFINITY && u != src) break;
        // u entra al árbol
        inMST[u] = 1;
 
        // aristas de u en el formato CSR: edges[offsets[u] .. offsets[u+1]-1]
        uint64_t start = graph->offsets[u];
        uint64_t end   = graph->offsets[u+1];
 
        for(uint64_t i = start; i < end; i++) {
            uint32_t v = graph->edges[i];       // vecino
            double   w = (double)graph->weights[i]; // peso de la arista {u, v}
            // solo se mejora a vecinos que siguen en la cola y con una arista más barata
            if(!inMST[v] && w < key[v]) {
                parent[v] = u;                  // v se conectaría al árbol mediante u
                // decreaseKey asigna key[v] = w y reordena la cola (corte y corte en cascada)
                decreaseKey_Fibo32(q, key, v, w, flags);
            }
        }
    }
    // libera inMST (y flags, que es parte del mismo bloque) y la cola
    free(inMST);
    free(q);
 
    dest->nodeCount = n;
    return dest;
}