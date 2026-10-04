
/**
 * Graphs_Prim.test.c
 * Casos de prueba de Prim_Binomial32 con grafos conexos de 5, 10, 15, 20 y 25 nodos.
 *
 * Cómo se verificó el MST "a mano" (valor esperado):
 *  - Todos los pesos de cada grafo son DISTINTOS, por lo que el MST es ÚNICO
 *    (así se puede comparar arista por arista, y no solo el peso total).
 *  - Cada arista lleva un comentario: "MST" o "descartada". Una arista descartada
 *    cumple la propiedad del ciclo: es la más pesada de algún ciclo, en concreto
 *    es estrictamente más pesada que la mayor arista del camino entre sus extremos
 *    en el MST (esa arista máxima se indica en el comentario). Verificar esto para
 *    cada arista descartada prueba que el MST propuesto es el mínimo.
 *  - Tras cada grafo se lista el MST ordenado por peso (orden de Kruskal) y su total.
 *  - PARENT/KEY es ese MST enraizado en el nodo 0 (lo que debe devolver Prim con src = 0).
 *
 * El grafo de 5 nodos (G5) tiene además la traza completa de Prim.
 *
 * Compilar (desde la carpeta del proyecto):
 *   gcc -std=c2x -Wall -Wextra -g -I. Graphs_Prim.test.c Graphs.c -lm -o prim_test
 *   ./prim_test
 * (opcional: agregar -fsanitize=address,undefined)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <Graphs.h>

/* ------------------------------------------------------------------ */
/* Utilidades                                                          */
/* ------------------------------------------------------------------ */

typedef struct { uint32_t u, v; double w; } Edge;

static int failures = 0;

#define CHECK(cond, ...) do { \
    if(!(cond)) { failures++; printf("    [FALLO] "); printf(__VA_ARGS__); printf("\n"); } \
} while(0)

/** Construye un WGraph32 (CSR, no dirigido) a partir de una lista de aristas no dirigidas. */
static WGraph32* buildGraph(uint32_t n, const Edge* edges, uint32_t m) {
    WGraph32* g = malloc(sizeof(WGraph32));
    g->nodeCount = n;
    g->offsets = calloc(n + 1, sizeof(uint64_t));
    g->edges   = malloc(2 * (size_t)m * sizeof(uint32_t));
    g->weights = malloc(2 * (size_t)m * sizeof(double));

    for(uint32_t i = 0; i < m; i++) {           // grados
        g->offsets[edges[i].u + 1]++;
        g->offsets[edges[i].v + 1]++;
    }
    for(uint32_t i = 0; i < n; i++) g->offsets[i + 1] += g->offsets[i];   // prefix sums

    uint64_t* pos = malloc(n * sizeof(uint64_t));
    for(uint32_t i = 0; i < n; i++) pos[i] = g->offsets[i];
    for(uint32_t i = 0; i < m; i++) {
        uint64_t a = pos[edges[i].u]++, b = pos[edges[i].v]++;
        g->edges[a] = edges[i].v; g->weights[a] = edges[i].w;
        g->edges[b] = edges[i].u; g->weights[b] = edges[i].w;
    }
    free(pos);
    return g;
}

static void freeGraph(WGraph32* g) { free(g->offsets); free(g->edges); free(g->weights); free(g); }

/** ¿{a, b} con peso w es una arista del MST esperado? (se lee de PARENT/KEY enraizado en 0) */
static int isExpectedEdge(const uint32_t* expParent, const double* expKey, uint32_t a, uint32_t b, double w) {
    return (expParent[a] == b && expKey[a] == w) || (expParent[b] == a && expKey[b] == w);
}

/**
 * Ejecuta Prim desde @p src y compara con el MST esperado.
 * - src == 0: se compara parent[] y key[] exactamente con los esperados.
 * - src != 0: el arbol es el mismo pero con otra raíz; se compara el conjunto de aristas y el peso total.
 * Devuelve el peso total obtenido.
 */
static double runAndCompare(const char* name, uint32_t n, const Edge* edges, uint32_t m,
                            const uint32_t* expParent, const double* expKey, double expTotal, uint32_t src) {
    WGraph32* g = buildGraph(n, edges, m);
    MST32 mst = { n, malloc(n * sizeof(uint32_t)), malloc(n * sizeof(double)) };
    // se envenena la salida para detectar campos que Prim no escriba
    for(uint32_t i = 0; i < n; i++) { mst.parent[i] = 0xDEADBEEF; mst.key[i] = -1.0; }

    MST32* r = Prim_Binomial32(g, src, &mst);
    CHECK(r == &mst, "%s (src=%u): Prim_Binomial32 no devolvio dest", name, src);

    double total = 0.0;
    if(r == &mst) {
        CHECK(mst.parent[src] == ROOT && mst.key[src] == 0.0, "%s (src=%u): la raiz debe tener parent=ROOT y key=0", name, src);
        for(uint32_t v = 0; v < n; v++) {
            if(v == src) continue;
            total += mst.key[v];
            if(mst.parent[v] >= n) { CHECK(0, "%s (src=%u): nodo %u sin padre valido (%u)", name, src, v, mst.parent[v]); continue; }
            CHECK(isExpectedEdge(expParent, expKey, v, mst.parent[v], mst.key[v]),
                  "%s (src=%u): arista {%u,%u}=%g no pertenece al MST esperado", name, src, v, mst.parent[v], mst.key[v]);
            if(src == 0) {
                CHECK(mst.parent[v] == expParent[v], "%s: parent[%u] = %u, esperado %u", name, v, mst.parent[v], expParent[v]);
                CHECK(mst.key[v] == expKey[v],       "%s: key[%u] = %g, esperado %g", name, v, mst.key[v], expKey[v]);
            }
        }
        CHECK(total == expTotal, "%s (src=%u): peso total = %g, esperado %g", name, src, total, expTotal);
    }
    if(src == 0 && r == &mst) {
        printf("    MST obtenido (nodo: padre [peso]): ");
        for(uint32_t v = 1; v < n; v++) printf("%u:%u[%g] ", v, mst.parent[v], mst.key[v]);
        printf("\n");
    }
    free(mst.parent); free(mst.key); freeGraph(g);
    return total;
}

#define RUN_CASE(NAME, N, TOTAL) do { \
    uint32_t m = sizeof(NAME##_EDGES) / sizeof(NAME##_EDGES[0]); \
    int before = failures; \
    printf("\n[%s] %u nodos, %u aristas. Peso total esperado = %g\n", #NAME, (uint32_t)(N), m, (double)(TOTAL)); \
    double t0 = runAndCompare(#NAME, N, NAME##_EDGES, m, NAME##_PARENT, NAME##_KEY, TOTAL, 0); \
    printf("    Peso total obtenido (src=0)    = %g\n", t0); \
    runAndCompare(#NAME, N, NAME##_EDGES, m, NAME##_PARENT, NAME##_KEY, TOTAL, (N) - 1); \
    runAndCompare(#NAME, N, NAME##_EDGES, m, NAME##_PARENT, NAME##_KEY, TOTAL, (N) / 2); \
    printf("    src = 0, %u y %u: %s\n", (uint32_t)((N) - 1), (uint32_t)((N) / 2), failures == before ? "OK" : "FALLO"); \
} while(0)

/* ------------------------------------------------------------------ */
/* Datos                                                               */
/* ------------------------------------------------------------------ */

/*
 * ---- G5: verificación completa a mano ----
 * Kruskal (aristas por peso): {1,2}=2 ok | {2,3}=3 ok | {0,1}=4 ok | {3,4}=5 ok  -> 4 aristas = V-1, fin.
 *   total = 2 + 3 + 4 + 5 = 14
 * Descartadas: {1,3}=6 (ciclo 1-2-3, max 3), {0,2}=8 (ciclo 0-1-2, max 4), {2,4}=9 (ciclo 2-3-4, max 5)
 *
 * Traza de Prim desde 0  (key = [0, inf, inf, inf, inf]):
 *   extrae 0 (0): key[1]=4 (p=0), key[2]=8 (p=0)
 *   extrae 1 (4): key[2]=2 (p=1) mejora 8;  key[3]=6 (p=1)
 *   extrae 2 (2): key[3]=3 (p=2) mejora 6;  key[4]=9 (p=2)
 *   extrae 3 (3): key[4]=5 (p=3) mejora 9
 *   extrae 4 (5): fin.   parent = [ROOT,0,1,2,3]   key = [0,4,2,3,5]   total = 14
 */
/* ---- 5 nodos, 7 aristas ---- */
static const Edge G5_EDGES[] = {
    {0, 1, 4.0}, /* MST */
    {0, 2, 8.0}, /* descartada */
    {1, 2, 2.0}, /* MST */
    {1, 3, 6.0}, /* descartada */
    {2, 3, 3.0}, /* MST */
    {2, 4, 9.0}, /* descartada */
    {3, 4, 5.0}, /* MST */
};
static const uint32_t G5_PARENT[] = {ROOT, 0, 1, 2, 3};
static const double   G5_KEY[]    = {0.0, 4.0, 2.0, 3.0, 5.0};
/* MST (Kruskal): {1,2}=2 {2,3}=3 {0,1}=4 {3,4}=5  -> total = 14 */

/* ---- 10 nodos, 14 aristas ---- */
static const Edge G10_EDGES[] = {
    {3, 7, 35.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,5}=33 < 35 */
    {2, 1, 36.0}, /* MST */
    {0, 7, 38.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,5}=33 < 38 */
    {1, 9, 37.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,9}=31 < 37 */
    {4, 0, 40.0}, /* MST */
    {0, 3, 34.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {1,3}=30 < 34 */
    {1, 0, 29.0}, /* MST */
    {7, 5, 12.0}, /* MST */
    {3, 1, 30.0}, /* MST */
    {6, 3, 13.0}, /* MST */
    {9, 6, 31.0}, /* MST */
    {6, 1, 32.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {3,1}=30 < 32 */
    {5, 0, 33.0}, /* MST */
    {8, 4, 39.0}, /* MST */
};
static const uint32_t G10_PARENT[] = {ROOT, 0, 1, 1, 0, 0, 3, 5, 4, 6};
static const double   G10_KEY[]    = {0.0, 29.0, 36.0, 30.0, 40.0, 33.0, 13.0, 12.0, 39.0, 31.0};
/* MST (Kruskal): {5,7}=12 {3,6}=13 {0,1}=29 {1,3}=30 {6,9}=31 {0,5}=33 {1,2}=36 {4,8}=39 {0,4}=40  -> total = 263 */

/* ---- 15 nodos, 23 aristas ---- */
static const Edge G15_EDGES[] = {
    {12, 10, 60.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,1}=59 < 60 */
    {3, 8, 51.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,8}=45 < 51 */
    {2, 1, 9.0}, /* MST */
    {9, 0, 8.0}, /* MST */
    {6, 4, 29.0}, /* MST */
    {7, 2, 12.0}, /* MST */
    {5, 14, 52.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,8}=45 < 52 */
    {11, 9, 42.0}, /* MST */
    {5, 1, 50.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {5,2}=40 < 50 */
    {1, 8, 49.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,8}=45 < 49 */
    {8, 6, 45.0}, /* MST */
    {5, 2, 40.0}, /* MST */
    {4, 1, 2.0}, /* MST */
    {1, 14, 54.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,8}=45 < 54 */
    {3, 10, 53.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {4,10}=48 < 53 */
    {12, 9, 23.0}, /* MST */
    {13, 8, 6.0}, /* MST */
    {14, 8, 15.0}, /* MST */
    {6, 1, 34.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,4}=29 < 34 */
    {1, 7, 28.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {2,7}=12 < 28 */
    {1, 0, 59.0}, /* MST */
    {3, 1, 16.0}, /* MST */
    {10, 4, 48.0}, /* MST */
};
static const uint32_t G15_PARENT[] = {ROOT, 0, 1, 1, 1, 2, 4, 2, 6, 0, 4, 9, 9, 8, 8};
static const double   G15_KEY[]    = {0.0, 59.0, 9.0, 16.0, 2.0, 40.0, 29.0, 12.0, 45.0, 8.0, 48.0, 42.0, 23.0, 6.0, 15.0};
/* MST (Kruskal): {1,4}=2 {8,13}=6 {0,9}=8 {1,2}=9 {2,7}=12 {8,14}=15 {1,3}=16 {9,12}=23 {4,6}=29 {2,5}=40 {9,11}=42 {6,8}=45 {4,10}=48 {0,1}=59  -> total = 354 */

/* ---- 20 nodos, 31 aristas ---- */
static const Edge G20_EDGES[] = {
    {8, 18, 70.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,0}=69 < 70 */
    {3, 5, 75.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {1,0}=74 < 75 */
    {18, 5, 20.0}, /* MST */
    {8, 13, 78.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,1}=74 < 78 */
    {10, 19, 66.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {5,0}=62 < 66 */
    {18, 6, 72.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,6}=69 < 72 */
    {7, 3, 68.0}, /* MST */
    {15, 14, 10.0}, /* MST */
    {9, 6, 65.0}, /* MST */
    {12, 0, 77.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {12,10}=76 < 77 */
    {19, 5, 73.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,5}=62 < 73 */
    {19, 9, 79.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,6}=69 < 79 */
    {6, 0, 69.0}, /* MST */
    {16, 14, 39.0}, /* MST */
    {2, 1, 22.0}, /* MST */
    {5, 0, 62.0}, /* MST */
    {10, 16, 63.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {5,0}=62 < 63 */
    {4, 3, 36.0}, /* MST */
    {11, 1, 80.0}, /* MST */
    {10, 5, 42.0}, /* MST */
    {14, 0, 37.0}, /* MST */
    {3, 1, 30.0}, /* MST */
    {0, 19, 38.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,14}=37 < 38 */
    {13, 3, 57.0}, /* MST */
    {2, 4, 55.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {3,4}=36 < 55 */
    {8, 6, 24.0}, /* MST */
    {6, 19, 71.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {6,0}=69 < 71 */
    {1, 0, 74.0}, /* MST */
    {19, 15, 34.0}, /* MST */
    {12, 10, 76.0}, /* MST */
    {17, 10, 53.0}, /* MST */
};
static const uint32_t G20_PARENT[] = {ROOT, 0, 1, 1, 3, 0, 0, 3, 6, 6, 5, 1, 10, 3, 0, 14, 14, 10, 5, 15};
static const double   G20_KEY[]    = {0.0, 74.0, 22.0, 30.0, 36.0, 62.0, 69.0, 68.0, 24.0, 65.0, 42.0, 80.0, 76.0, 57.0, 37.0, 10.0, 39.0, 53.0, 20.0, 34.0};
/* MST (Kruskal): {14,15}=10 {5,18}=20 {1,2}=22 {6,8}=24 {1,3}=30 {15,19}=34 {3,4}=36 {0,14}=37 {14,16}=39 {5,10}=42 {10,17}=53 {3,13}=57 {0,5}=62 {6,9}=65 {3,7}=68 {0,6}=69 {0,1}=74 {10,12}=76 {1,11}=80  -> total = 898 */

/* ---- 25 nodos, 40 aristas ---- */
static const Edge G25_EDGES[] = {
    {14, 8, 74.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {3,2}=70 < 74 */
    {8, 6, 29.0}, /* MST */
    {16, 3, 80.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {2,3}=70 < 80 */
    {11, 8, 93.0}, /* MST */
    {10, 15, 78.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {10,13}=73 < 78 */
    {21, 4, 91.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,4}=90 < 91 */
    {16, 8, 69.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {1,0}=53 < 69 */
    {23, 10, 42.0}, /* MST */
    {15, 13, 13.0}, /* MST */
    {15, 2, 85.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {13,10}=73 < 85 */
    {1, 22, 86.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {8,19}=82 < 86 */
    {14, 12, 99.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {10,12}=96 < 99 */
    {12, 10, 96.0}, /* MST */
    {13, 10, 73.0}, /* MST */
    {14, 3, 2.0}, /* MST */
    {7, 0, 49.0}, /* MST */
    {19, 8, 82.0}, /* MST */
    {9, 4, 38.0}, /* MST */
    {7, 19, 88.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {8,19}=82 < 88 */
    {3, 2, 70.0}, /* MST */
    {17, 15, 75.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {10,13}=73 < 75 */
    {23, 17, 98.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {2,1}=67 < 98 */
    {21, 16, 76.0}, /* MST */
    {24, 17, 46.0}, /* MST */
    {14, 2, 84.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {3,2}=70 < 84 */
    {8, 4, 92.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,4}=90 < 92 */
    {15, 9, 97.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {0,4}=90 < 97 */
    {10, 2, 4.0}, /* MST */
    {4, 0, 90.0}, /* MST */
    {1, 0, 53.0}, /* MST */
    {16, 1, 21.0}, /* MST */
    {5, 2, 15.0}, /* MST */
    {2, 1, 67.0}, /* MST */
    {22, 19, 52.0}, /* MST */
    {20, 2, 94.0}, /* MST */
    {6, 0, 23.0}, /* MST */
    {17, 8, 66.0}, /* MST */
    {8, 18, 95.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {10,13}=73 < 95 */
    {12, 9, 100.0}, /* descartada: ciclo con el MST, arista mas pesada del ciclo {12,10}=96 < 100 */
    {18, 15, 39.0}, /* MST */
};
static const uint32_t G25_PARENT[] = {ROOT, 0, 1, 2, 0, 2, 0, 0, 6, 4, 2, 8, 10, 10, 3, 13, 1, 8, 15, 8, 2, 16, 19, 10, 17};
static const double   G25_KEY[]    = {0.0, 53.0, 67.0, 70.0, 90.0, 15.0, 23.0, 49.0, 29.0, 38.0, 4.0, 93.0, 96.0, 73.0, 2.0, 13.0, 21.0, 66.0, 39.0, 82.0, 94.0, 76.0, 52.0, 42.0, 46.0};
/* MST (Kruskal): {3,14}=2 {2,10}=4 {13,15}=13 {2,5}=15 {1,16}=21 {0,6}=23 {6,8}=29 {4,9}=38 {15,18}=39 {10,23}=42 {17,24}=46 {0,7}=49 {19,22}=52 {0,1}=53 {8,17}=66 {1,2}=67 {2,3}=70 {10,13}=73 {16,21}=76 {8,19}=82 {0,4}=90 {8,11}=93 {2,20}=94 {10,12}=96  -> total = 1233 */


/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("--------TESTS Prim_Binomial32 (grafos de 5, 10, 15, 20 y 25 nodos)-------\n");

    RUN_CASE(G5,   5,   14);
    RUN_CASE(G10,  10,  263);
    RUN_CASE(G15,  15,  354);
    RUN_CASE(G20,  20,  898);
    RUN_CASE(G25,  25,  1233);

    printf("\n");
    if(failures == 0) { printf("--------TODOS LOS TESTS PASARON-------\n"); return 0; }
    printf("--------%d VERIFICACIONES FALLARON-------\n", failures);
    return 1;
}