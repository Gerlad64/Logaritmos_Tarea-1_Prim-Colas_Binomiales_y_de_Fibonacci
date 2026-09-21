
#include <stdint.h>

/**
 * @brief Grafo no dirigido representado mediante arreglos.
 *
 * Almacena únicamente la topología de adyacencia sin pesos ni valores asociados.
 * Los identificadores de nodos son contiguos en el rango [0, nodeCount - 1].
 *
 * @details
 * Al ser un grafo no dirigido, cada arista {u, v} se almacena dos veces en @p edges:
 * una en la lista de adyacencia de @c u y otra en la de @c v. Por ende, @p edgeCount
 * equivale al doble del número real de aristas no dirigidas.
 *
 * Los vecinos de un nodo @c u se encuentran en:
 * @code
 * uint32_t start = graph->offsets[u];
 * uint32_t end   = (u + 1 < graph->nodeCount) ? graph->offsets[u + 1] : graph->edgeCount;
 * // Vecinos de u: edges[start ... end - 1]
 * // Grado de u:   end - start
 * @endcode
 */
typedef struct {
    /**
     * @brief Número total de nodos en el grafo.
     * Los índices van desde 0 a nodeCount-1
     */
    uint16_t nodeCount;
    /**
     * @brief Lista de adyacencia de todos los nodos.
     * Contiene los identificadores de los nodos vecinos agrupados
     * por nodo de origen.
     * Su longitud exacta está dada por @p graph->offsets[node]
     * @note A priori la lista y los vecinos no siguen ningún orden en especifico.
     */
    uint16_t* edges;
    /**
        Offsets de aristas. Su tamaño es igual a `nodeCount`, 
        donde el índice i permite acceder a los vecinos del 
        nodo i consultando el arreglo `edges`.
        Por esto, los offsets **deben estar ordenados por nodo**.
    */
    uint32_t* offsets;
} Graph16;

typedef struct {
    uint32_t nodeCount;
    uint32_t* edges;
    uint64_t* offsets;
} Graph32;

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
    uint64_t* weights;
} WGraph32;

/** 
 * @struct BinomialTreeForest32
 * @brief Bosque de Arboles Binomiales representada mediante arreglos.
 * 
 * Almacena la topología de un bosque de arboles binomiales mediante un arreglo de padres y un arreglo de las raíces.
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
 * @note
 * El orden y distribución de las raíces en el arreglo de raíces depende de cada uso
 */
typedef struct {
    /** Número total de nodos en el bosque. */
    uint32_t nodeCount; 
    /** Número total de raíces en el bosque. */
    uint32_t rootCount;
    /** Arreglo de tamaño @p nodeCount que asigna el índice de un nodo con su padre. */
    uint32_t* parents;
    /** Arreglo que contiene los índices que son raíces. */
    uint32_t* roots;
} BinomialTreeForest32;

constexpr uint32_t ROOT = (uint32_t)(-1);
constexpr uint32_t FREE = (uint32_t)(-1);

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
BinomialTreeForest32* initFiboQueue32(uint32_t nodeCount, BinomialTreeForest32* dest);

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
        .parents = (uint32_t[N]), \
        .roots  = (uint32_t[32]) \
    })
    
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
        .parents = (uint32_t[N]), \
        .roots = (uint32_t[N]) \
    })