
#include <Graphs.h>
#include <stdint.h>


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
            dest->parents[carryNode] = currentRoot;
            addChildren(dest, currentRoot, carryNode);
            carryNode = currentRoot;
            carryValue = nodeValues[currentRoot];
        }
        else {
            // carryNode es menor o igual: adopta a currentRoot
            // y este pierde su estatus de raíz
            dest->parents[currentRoot] = carryNode;
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
        if(currentRoot != FREE && values[currentRoot] < values[dest->minNode]) {
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
        temp = values[y];
        values[y] = values[x];
        values[x] = temp;

        x = y;
        y = dest->parents[x];
    }
    if( values[x] < values[dest->minNode]) {
        dest->minNode = x;
    }
}