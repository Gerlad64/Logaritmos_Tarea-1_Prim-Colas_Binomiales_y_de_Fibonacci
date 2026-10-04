#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <WGraphs.h>

/**
 * @struct BinomialTreeForest32
 * @brief Bosque de Arboles Binomiales representada mediante arreglos.
 *
 * Almacena la topología de un bosque de arboles binomiales mediante un arreglo de padres, un arreglo de las raíces
 * un arreglo de hijos de cada nodo, y un arreglo del grado de cada arbol.
 * No almacena los valores de los nodos.
 *
 * @details
 * El padre de un nodo @c u se obtiene consultando el arreglo @p parents:
 * @code
 * uint32_t u_parent = bintreeforest.parents[u];
 * @endcode
 *
 * Si un nodo @c u es una raíz, su valor en el arreglo @p parents se identifiacará como @c ROOT
 * @code
 * bintreeforest.parentes[u] == ROOT == (uint32_t)(-1)
 * @endcode
 *
 * El arreglo @p roots almacena los índices de los nodos raíz del bosque.
 * La cantidad de raíces está dada por @p rootCount, aunque la capacidad del
 * arreglo @p roots puede ser mayor.
 * @code
 * /// raíces del bosque:
 * roots[0 ... rootCount]
 * @endcode
 *
 * El arreglo @c children ocupa un espacio constante de 32 bits por cada nodo, que
 * representa la cantidad máxima de hijos que puede tener un nodo en un sistema de nodos
 * de máximo 32 bits. Entonces, el arreglo children toma N * 32 en memoria, y los arreglos
 * de hijos se encuentran separados con un *stride* constante.
 * 
 * La cantidad de hijos de un nodo está dada por su grado, almacenado en el arreglo @p deg.
 * 
 * Los hijos de un nodo @c u se obtiene consultando el arreglo @p children y el arreglo @p deg:
 * @code
 * uint64_t firstChild = (uint64_t)u * 32;
 * uint64_t lastChild  = firstChild + bintreeforest.deg[u];
 * /// hijos del nodo u
 * children[firstChild ... lastChild]
 * @endcode
 * 
 * El índice del nodo con el valor mínimo  es almacenado en @p minNode
 * 
 * @note
 * El orden y distribución de las raíces en el arreglo de raíces depende de cada uso
 */
typedef struct {
    /** Número total de nodos en el bosque. */
    uint32_t nodeCount;
    /** Número total de raíces en el bosque. */
    uint32_t rootCount;
    /** Índice del nodo con el valor mínimo */
    uint32_t minNode;
    /** Arreglo de tamaño @p nodeCount que asigna el índice de un nodo con su padre. */
    uint32_t* parents;
    /** Arreglo que contiene los índices que son raíces. */
    uint32_t* roots;
    /** Arreglo de tamaño @p nodeCount*32 que almacena los índices de los hijos de cada nodo
        con *stride* constante de 32.
    */
    uint32_t* children;
    /** Arreglo que contiene el grado de cada arbol almacenado */
    uint32_t* deg;
} BinomialTreeForest32;

constexpr uint32_t ROOT = (uint32_t)(-1);
constexpr uint32_t FREE = (uint32_t)(-1);
constexpr uint32_t NO_CHILD = (uint32_t)(-1);

/**
 * @brief Añade a @p p un hijo @p c en la estructura @p dest
 * 
 * @details
 * Modifica adecuadamente la estructura @p dest para que @p p aumento su grado en 1,
 * tenga a @p c en su lista de hijos y @p c tenga a @p p en la lista de @c parents
 * 
 * @param dest Estructura de destino a ser modificada
 * @param p Nodo que será padre (parent)
 * @param c Nodo que será hijo  (child)
 */
static inline void addChildren(BinomialTreeForest32* dest, uint32_t p, uint32_t c) {
    uint64_t childIndex = (uint64_t)p * 32 + dest->deg[p];
    dest->parents[c] = p;
    dest->children[childIndex] = c;
    dest->deg[p] += 1;
}

/**
 * @brief Intercambia de lugar un nodo padre @p p con uno de sus nodos hijos @p c y propaga los cambios
 * en la estructura @p dest
 * 
 * @details
 * El intercambio entre @p p y @p c se realiza:
 * 1. Intercambiando a sus hijos
 * 2. Invirtiendo la relación parent child entre @p y @c, es decir, @p c tiene a @p p en su lista de
 *    hijos y @p p tiene a @p c como padre.
 * 3. Actualizar al abuelo original de @p c, quien ahora es su padre, y @c gp ahora tiene a @p c en
 *    su lista de hijos
 * 
 */
static inline void swapParentWithChild(BinomialTreeForest32* dest, uint32_t p, uint32_t c) {
    // intercambiar de lugar p y c
    uint32_t gp = dest->parents[p]; // grandparent
    uint32_t degP = dest->deg[p];
    uint32_t degC = dest->deg[c];
    // c es de menor grado
    // índice hijos de c
    uint64_t cindex = (uint64_t)c * 32;
    // índice hijos de p
    uint64_t pindex = (uint64_t)p * 32;

    // guardar copias de los hijos de c y de p
    uint32_t oldP[32], oldC[32];
    for(uint32_t d = 0; d < degP; d++) oldP[d] = dest->children[pindex + d];
    for(uint32_t d = 0; d < degC; d++) oldC[d] = dest->children[cindex + d];

    // actualizar hijos de c con los hijos de p
    for(uint32_t d = 0; d < degP; d++) {
        uint32_t child = oldP[d];
        dest->children[cindex + d] = child;
        dest->parents[child] = c;
    }
    // ahora p es hijo de c
    dest->children[cindex + degC] = p;
     
    // actualizar hijos de p con los hijos de c
    for(uint32_t d = 0; d < degC; d++) {
        uint32_t child = oldC[d];
        dest->children[pindex + d] = child;
        dest->parents[child] = p;
    }
    // actualizar grados (swap)
    dest->deg[c] = degP;
    dest->deg[p] = degC;

    dest->parents[p] = c;
    dest->parents[c] = gp;

    // actualizar al abuelo
    if(gp == ROOT) {
        dest->roots[degP] = c;
    }
    else {
        uint64_t gpindex = (uint64_t)gp * 32;
        //uint32_t gpDeg = dest->deg[gp];
        dest->children[gpindex + degP] = c;
    }
}

/** 
 * @brief Saca a x de la lista de hijos y y decrementar el grado de y, luego
 * agrega a x a la lista de árboles de Y.
 * 
 */
static inline void cut(BinomialTreeForest32* dest, uint32_t x, uint32_t y, uint8_t *flags) {
    uint64_t childIndex = (uint64_t)y*32;
    uint64_t last = childIndex + dest->deg[y];
    // buscar a x dentro de los hijos de y
    while(childIndex < last && dest->children[childIndex] != x) childIndex++;
    // no se encontró x
    if(childIndex == last) return; 
     
    // Se elimina x
    dest->children[childIndex] = dest->children[last-1];
    dest->deg[y]--;
    
    // Se coloca x en la raíz
    dest->parents[x] = ROOT;
    // se desmarca x al estar recién cortado
    flags[x] = 0;
    // se coloca en la última raíz, 
    // suponiendo que queda espacio, ya que se llama
    // después de extractMin
    dest->roots[dest->rootCount++] = x;
};

static inline void cascadingCut(BinomialTreeForest32* dest, uint32_t y, uint8_t *flags) {
    uint32_t z = dest->parents[y];
    while( z != ROOT ) {
        if(flags[y] == 0) {
            flags[y] = 1;
            break;
        }
        else {
            cut(dest, y, z, flags);
            y = z;
            z = dest->parents[z];
        }
    }
}


/**
 * @brief *Acarrea* el nodo @p carryNode de grado @p degree (es decir, cuantos hijos tiene) sobre la estructura @p dest 
 * 
 * @param dest Estructura de destino que será modificada
 * @param nodeValues Arreglo de valores que será consultado para comparar valores y hacer carry
 * @param carryNode Nodo que será *acarreado* en la estructura
 * @param degree Grado del nodo a *acarrear*
 */
void carry(BinomialTreeForest32* dest, const double* nodeValues, uint32_t carryNode, uint32_t degree);

/**
 * @brief Inicializa una Cola Binomial insertando valores secuencialmente.
 *
 * @details
 * Configura los arreglos @p parents y @p roots del bosque de destino para que adopte la topología
 * de una Cola Binomial a partir de los valores dados.
 *
 * @param nodeCount Cantidad de nodos a insertar
 * @param nodeValues Arreglo de tamaño @c nodeCount con los valores o prioridades asociados a los nodos
 * @param dest Puntero a la estructura @p dest previamente alocada que será inicializada
 * @return Puntero a la estructura @p dest inicializada
 *
 */
BinomialTreeForest32* initBinomialQueue32(uint32_t nodeCount, const double* nodeValues, BinomialTreeForest32* dest);

/**
 * @brief Inicializa una Cola de Fibonacci insertando valores secuencialmente.
 *
 * @details
 * Inserta de manera secuencial los valores dados en @c nodeValues[0 .. nodeCount].
 * En una cola de Fibonacci esto significa agregar @c nodeCount árboles B0 a una cola vacía,
 * por lo que los arreglos @p parents y @p roots configura a todos los nodos como raíces.
 *
 * @param nodeCount Cantidad de nodos a insertar
 * @param nodeValues Arreglo de tamaño @c nodeCount con los valores o prioridades asociados a los nodos
 * @param dest Puntero a una estructura @p dest previamente alocada que será inicializada
 * @return Puntero a la estructura @p dest inicializada
 *
 */
BinomialTreeForest32* initFiboQueue32(uint32_t nodeCount, const double* nodeValues, BinomialTreeForest32* dest);


/**
 * @brief Extra el nodo de valor mínimo de la Cola Binomial @p dest y retorna su identificador.
 * 
 * @param dest Cola Binomial que será modificada para tener el nodo de valor mínimo extraido.
 * @param values Arreglo de valores de tamaño @c dest->nodeCount, usado para consultar los valores de los nodos
 * 
 * @returns Identificador del nodo extraído, su valor se consulta al arreglos @p values : @c values[x]
 */
uint32_t extractMin_Binomial32(BinomialTreeForest32* dest, const double* values);

/**
 * @brief Extra el nodo de valor mínimo de la Cola de Fibonacci @p dest y retorna su identificador.
 * 
 * @param dest Cola de Fibonacci que será modificada para tener el nodo de valor mínimo extraido.
 * @param values Arreglo de valores de tamaño @c dest->nodeCount, usado para consultar los valores de los nodos
 * 
 * @returns Identificador del nodo extraído, su valor se consulta al arreglos @p values : @c values[x]
 */
uint32_t extractMin_Fibo32(BinomialTreeForest32* dest, const double* values);


/**
 * @brief Modifica la Cola Binomial @p dest para que mantenga su estructura 
 * luego de reducir a @p cost el costo actual del nodo identificado por @p x
 * 
 * @param dest Cola Binomial que será modificada para decrementar el costo de un nodo
 * @param values Arreglo de valores de tamaño @c dest->nodeCount, usado para consultar los valores
 * de los nodos.
 * @param node Identificador del nodo que decrementará su costo
 * @param cost Nuevo costo del nodo
 */
void decreaseKey_Binomial32(BinomialTreeForest32* dest, double* values, uint32_t x, double cost);

/**
 * @brief Modifica la Cola de Fibonacci @p dest para que mantenga su estructura 
 * luego de reducir a @p cost el costo actual del nodo identificado por @p x.
 * 
 * @param dest Cola de Fibonacci que será modificada para decrementar el costo de un nodo
 * @param values Arreglo de valores de tamaño @c dest->nodeCount, usado para consultar los valores
 * de los nodos.
 * @param node Identificador del nodo que decrementará su costo
 * @param cost Nuevo costo del nodo
 */
void decreaseKey_Fibo32(BinomialTreeForest32* dest, double* values, uint32_t x, double cost, uint8_t *flags);

/**
 * @macro STACK_BINOMIAL_QUEUE
 * @brief Instancia e inicializa una Cola Binomial utilizando memoria de *stack*.
 *
 * @note El arreglo de raíces reserva espacio para 32 elementos, ya que un bosque binomial
 * con identificadores de 32 bits sin signo no puede tener más de 32 raíces.
 *
 * @param N número de nodos (@c nodeCount)
 * @param VALUES puntero al arreglo de valores (@c nodeValues)
 * @return Puntero a una estructura @c BinomialTreeForest32 creada en el stack.
 */
#define STACK_BINOMIAL_QUEUE(N, VALUES) \
    initBinomialQueue32((N), (VALUES), &(BinomialTreeForest32){ \
        .nodeCount = (N), \
        .rootCount = (0), \
        .minNode = (FREE), \
        .parents = (uint32_t[(N)]){0}, \
        .roots  = (uint32_t[32]){0}, \
        .children = (uint32_t[(size_t)(N)*32]){0}, \
        .deg = (uint32_t[(N)]){0} \
    })

/**
 * @brief Instancia e inicializa una Cola Binomial utilizando memoria de heap.
 * 
 * @note El arreglo de raíces reserva espacio para 32 elementos, ya que un bosque binomial
 * con identificadores de 32 bits sin signo no puede tener más de 32 raíces.
 * @note Se debe liberar la memoria prestada con free
 * 
 * @param N número de nodos (@c nodeCount)
 * @param values puntero al arreglo de valores (@c nodeValues)
 * @return Puntero a una estructura @c BinomialTreeForest32 que vive en el heap.
 */
static inline BinomialTreeForest32* HEAP_BINOMIAL_QUEUE(uint32_t N, double* values) {
    size_t plen = N;            // arreglo parents
    size_t rlen = 32;           // arreglo roots
    size_t clen = (size_t)N*32; // arreglo children
    size_t dlen = N;            // arreglo deg
    size_t total_elements = plen + rlen + clen + dlen;
    size_t total_size = sizeof(BinomialTreeForest32) + sizeof(uint32_t) * total_elements;
    
    void *ptr = malloc(total_size);
    if (!ptr) return NULL;

    BinomialTreeForest32 *forest = (BinomialTreeForest32*) ptr;
    // avanza ptr exactamente sizeof(BinomialTreeForest32) bytes
    uint32_t *data = (uint32_t*)((char*)ptr + sizeof(BinomialTreeForest32));

    // inicializar campos numéricos
    forest->nodeCount = N;
    forest->rootCount = 0;
    forest->minNode = FREE;

    //inicializar punteros
    // debe mover el puntero data
    // para asignar correctamente cada campo
    // parents
    forest->parents = data;
    data += plen;
    //roots
    forest->roots = data;
    data += rlen;
    //children
    forest->children = data;
    data += clen; // recordar que esto es N log((uint32_t)(-1))
    // deg
    forest->deg = data;
    
    return initBinomialQueue32(N, values, forest);
}

/**
* @macro STACK_FIBO_QUEUE
* @brief Instancia e inicializa una Cola de Fibonacci utilizando memoria de *stack*.
*
* @note El arreglo de raíces reserva espacio para @c nodeCount elementos, ya que una
* cola de Fibonacci comienza con @c nodeCount árboles B0.
*
* @param N número de nodos (@c nodeCount)
* @param VALUES puntero al arreglo de valores (@c nodeValues)
* @return Puntero a una estructura @c BinomialTreeForest32 creada en el stack.
*/
#define STACK_FIBO_QUEUE(N) \
    initFiboQueue((N), &BinomialTreeForest32{ \
        .nodeCount = (N), \
        .rootCount = (N), \
        .minNode = (FREE), \
        .parents = (uint32_t[(N)]){0}, \
        .roots = (uint32_t[(N)]){0}, \
        .children = (uint32_t[(size_t)(N)*32]){0}, \
        .deg = (uint32_t[(N)]){0} \
    })

/**
* @brief Instancia e inicializa una Cola de Fibonacci utilizando memoria de heap.
* 
* @note El arreglo de raíces reserva espacio para N elementos, ya que una cola de fibonacci
* tiene permitido insertar elementos sin mantener la estructura de bosque binomial
* 
* @note Se debe liberar la memoria prestada con free
* 
* @param N número de nodos (@c nodeCount)
* @param values puntero al arreglo de valores (@c nodeValues)
* @return Puntero a una estructura @c BinomialTreeForest32 que vive en el heap.
*/
static inline BinomialTreeForest32* HEAP_FIBO_QUEUE(uint32_t N, double* values) {
    
    size_t plen = N;            // arreglo parents
    size_t rlen = N;           // arreglo roots
    size_t clen = (size_t)N*32; // arreglo children
    size_t dlen = N;            // arreglo deg
    size_t total_elements = plen + rlen + clen + dlen;
    size_t total_size = sizeof(BinomialTreeForest32) + sizeof(uint32_t) * total_elements;
    
    void *ptr = malloc(total_size);
    if (!ptr) return NULL;

    BinomialTreeForest32 *forest = (BinomialTreeForest32*) ptr;
    // avanza ptr exactamente sizeof(BinomialTreeForest32) bytes
    uint32_t *data = (uint32_t*)((char*)ptr + sizeof(BinomialTreeForest32));

    // inicializar campos numéricos
    forest->nodeCount = N;
    forest->rootCount = 0;
    forest->minNode = FREE;

    //inicializar punteros
    // debe mover el puntero data
    // para asignar correctamente cada campo
    // parents
    forest->parents = data;
    data += plen;
    //roots
    forest->roots = data;
    data += rlen;
    //children
    forest->children = data;
    data += clen; // recordar que esto es N log((uint32_t)(-1))
    // deg
    forest->deg = data;
    
    return initFiboQueue32(N, values, forest);
}

/**
 * @struct MST32
 * @brief Árbol Covertor Mínimo (MinimumSpanningTree) representado mediante arreglos 
 * 
 * @details
 * Los nodos se identifican con enteros csin signo en el rango [0, nodeCount - 1].
 * El árbol es representado en la lista @p parent, con la raíz del arbol siendo
 * el nodo elegido para comenzar el algoritmo de Prim.
 * 
 * Si @c v es un nodo del arbol, entonces, 
 * se consulta por su padre, y el peso de la arista que los une
 * de esta forma:
 * @code
 * uint32_t p = mst->parent[v]; // ROOT si v es la raíz o no fue alcanzado
 * double   w = mst->key[v]; // peso de la arista {v, p}
 * @endcode
 * 
 * Si el grafo es conexo, el peso total del MST es la suma de @c key[v] para todo @c v
 * distinto de la raíz.
 * 
 */
typedef struct {
    /** Número de nodos del MST, el mismo del grafo original si este es conexo */
    uint32_t nodeCount;
    /** Arreglo de tamaño @p nodeCount con el padre de cada nodo en el MST, o @c ROOT si no tiene */
    uint32_t* parent;
    /**
     * Arreglo de tamaño @p nodeCount con el peso de la arista que une a cada nodo con su padre.
     * Durante la ejecución de Prim también actúa como *clave* o prioridad de la cola
     */
    double *key;
} MST32;

static inline MST32* HEAP_MST32(uint32_t nodeCount) {
    size_t p_size = nodeCount * sizeof(uint32_t);
    size_t k_size = nodeCount * sizeof(double);
    
    void* ptr = malloc(sizeof(MST32) + p_size + k_size);
    if (!ptr) return NULL;
    
    MST32* m = (MST32*)ptr;
    char* data = (char*)ptr + sizeof(MST32);

    m->key = (double*)data;
    data += k_size;
    m->parent = (uint32_t*)data;
    m->nodeCount = nodeCount;

    return m;
}

/**
 * @brief Calcula un MST del grafo @p graph con el algoritmo de Prim, usando una Cola Binomial
 * como cola de prioridad.
 * 
 * @details
 * Prim hace crecer el arbol dese @p src, agregando en cada paso al nodo fuera del árbol
 * con la arista más barata hacia él. Cada nodo tiene una clave @c key[v]: el peso de la
 * arista más barata conocida que lo conecta con el árbol. La cola binomial almacena todos
 * los nodos ordenados por esa clave:
 * - se hace @c extractMin_Binomial32 una vez por nodo,
 * - se hace @c decreaseKey_Binomial32 cada vez que una arista mejora la clave de un vecino.
 * 
 * Complejidad: O(V log V) por las extracciones más la complejidad de decreaseKey,
 */
MST32* Prim_Binomial32(const WGraph32* graph, uint32_t src, MST32* dest);

/**
 * @brief Calcula un MST del grafo @p graph con el algoritmos de Prim, usando una Cola de Fibonacci
 * como cola de prioridad.
 * 
 * @details
 */
MST32* Prim_Fibo32(const WGraph32* graph, uint32_t src, MST32* dest);