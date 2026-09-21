
#include <Graphs.h>


BinomialTreeForest32* initBinomialQueue32(uint32_t nodeCount, const double* nodeValues, BinomialTreeForest32* dest) {
    dest->rootCount = 32;
    for(uint32_t i = 0; i < dest->rootCount; i++) dest->roots[i] = FREE;
    
    for(uint32_t i = 0; i < nodeCount; i++) {
        
        // nodo y valor para hacer carry
        uint32_t carryNode = i;
        double carryValue = nodeValues[i];
        // se realiza el proceso de carry recorriendo las raíces
        for(uint32_t r = 0; r < dest->rootCount; r++) {
            uint32_t currentRoot = dest->roots[r];
             
            // Si el espacio r está libre
            // ahora es ocupado por carryNode
            if( currentRoot == FREE ) {
                // colocar carryNode en una raíz
                dest->roots[r] = carryNode;
                dest->parents[carryNode] = ROOT;
                break;
            }
            // Si el espacio r está ocupado
            // se realiza la 'suma' de arboles
            
            // se escoge como nuevo valor de carry el de menor valor
            if( carryValue > nodeValues[currentRoot]) { // nodeValues[currentRoot] es un valor ya consultado
                dest->parents[carryNode] = currentRoot;
                carryNode = currentRoot;
                carryValue = nodeValues[currentRoot];
            }
            else {
                // carryNode es menor o igual: adopta a currentRoot
                // y este pierde su estatus de raíz
                dest->parents[currentRoot] = carryNode;
            }
            // se desaloja roots[r]
            dest->roots[r] = FREE;
        }
        
    }
    return dest; 
}

BinomialTreeForest32* initFiboQueue32(uint32_t nodeCount, const double* nodeValues, BinomialTreeForest32* dest) {
    return dest; 
}