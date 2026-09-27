
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


void decreaseKey_Binomial32(BinomialTreeForest32 *dest, double *values, uint32_t x, double cost) {
    values[x] = cost;
    uint32_t y = dest->parents[x];
    
    double temp;
    // si node está 
    while( y != ROOT && values[x] < values[y] ) {
        // values[node] <-> values[nodeDown]
        temp = values[y];
        values[y] = values[x];
        values[x] = temp;
        
        x = y;
        y = dest->parents[x];
    }
}