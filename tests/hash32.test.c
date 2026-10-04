/**
 * TESTS DE INTEGRIDAD DE HASH
 * prueba que tiene el comportamiento esperado para ser usado en la
 * generación de grafos aleatorios
 */

#include <stdio.h>
#include <assert.h>
#include <hash32.h>

void test_hash_behavior(void) {
    EdgeHash* table = hash_alloc(16, NULL);
    assert(table != NULL);

    assert(hash_try_insert(table, 3, 7) == 1);   /* nueva */
    assert(hash_try_insert(table, 3, 7) == 0);   /* repetida */
    assert(hash_try_insert(table, 7, 3) == 0);   /* {7,3} es la misma arista que {3,7} */
    assert(hash_try_insert(table, 0, 1) == 1);   /* distinta: incluye al nodo 0 */
    assert(hash_try_insert(table, 1, 0) == 0);
    free(table);

    /* Insertar maxEdges aristas distintas: todas deben aceptarse */
    uint64_t maxEdges = 1000;
    table = hash_alloc(maxEdges, NULL);
    for (uint32_t k = 1; k <= maxEdges; k++)
        assert(hash_try_insert(table, 0, k) == 1);
    for (uint32_t k = 1; k <= maxEdges; k++)     /* y todas deben detectarse como repetidas */
        assert(hash_try_insert(table, k, 0) == 0);
    free(table);
}

int main() {
    printf("--------TESTS hash32.h-------\n");
    test_hash_behavior(); 
    printf("--------TODOS LOS TESTS PASARON-------\n");
    return 0;
}