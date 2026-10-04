# Tarea 1 -- Algoritmo de Prim y Análisis Amortizado
**Integrantes: Gerald Ponce - Mariajose Severino**

Repositorio de github para la Tarea 1 del curso Diseño y Análisis de Algoritmos.


## Dependencias
Se requiere un compilador de C como clang o gcc y poder ejecutar Makefiles.

## Compilar y Ejecutar
**Se asegura el funcionamiento de los programas de este repositorio
para Linux y MacOS**, no se ha probado con Windows o WSL.

Para compilar y ejecutar los benchmarks pedidos (series A, B, C y D):
> make benchmark \<target\>

Donde `<target>` puede ser 1, 2, 3 para compilar con flag `O<target>`, y all
para ejecutar benchmarks con los 3 niveles de optmización uno tras otro.

Para compilar y ejecutar el programa main:
> make run

esto ejecuta el programa bonus de la tarea si es que está implementado.

Para compilar ejecutar los tests:
> make test

Para compilar y ejecutar un test en particular:
> make test \<name\>

En este caso, compila el archivo `<name>.test.c`.

Para limpiar los archivos compilados
> make clean

Para consultar la descripción de este y otros comandos ejecutar simplemente
> make