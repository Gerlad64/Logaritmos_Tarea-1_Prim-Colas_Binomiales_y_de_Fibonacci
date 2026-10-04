# compilador
CC = gcc
# Extraer la versión exacta del compilador dinámicamente
CC_VERSION := $(shell $(CC) --version | head -n 1)
#flags del compilador
CFLAGS = -I./include -std=c23

# Detectar Sistema Operativo
UNAME_S := $(shell uname -s)

# Definir directorios
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Ejecutables
MAIN = $(BIN_DIR)/main

# Archivos fuente (.c) y objetos (.o)
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# --- Variables para Testing ---
TEST_DIR = tests
TEST_BUILD_DIR = $(BUILD_DIR)/tests
TEST_BIN_DIR = $(BIN_DIR)/tests
 
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(TEST_BUILD_DIR)/%.o)
TEST_BINS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(TEST_BIN_DIR)/%)
 
# Filtramos main.o de los objetos del proyecto
OBJECTS_WITHOUT_MAIN = $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS))

# --- Variables para Benchmark ---
BENCH_DIR = benchmarks
BENCH_BIN_DIR = $(BIN_DIR)/benchmarks
BENCH_EXEC = $(BENCH_BIN_DIR)/bench

#----- Lógica para make test <nombre> ------
RUN_TEST_BINS = $(TEST_BINS)

ifeq (test,$(firstword $(MAKECMDGOALS)))
  TEST_ARGS := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  ifneq ($(TEST_ARGS),)
    $(eval $(TEST_ARGS):;@:)
    RUN_TEST_BINS = $(patsubst %,$(TEST_BIN_DIR)/%.test,$(TEST_ARGS))
  endif
endif

#----- Lógica para make benchmark <target> <custom> ------
# Uso:
#   make benchmark                          -> compila y ejecuta con cada nivel de OPT_LEVELS
#   make benchmark 3                        -> solo -O3
#   make -- benchmark all "-march=native"   -> todos los niveles con flags extra
#   make benchmark 2 CUSTOM="-march=native -flto"
# Niveles de optimización que recorre "benchmark" / "benchmark all"
OPT_LEVELS = 1 2 3

ifeq (benchmark,$(firstword $(MAKECMDGOALS)))
  BM_ARGS := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  BM_TARGET := $(word 1, $(BM_ARGS))

  ifeq ($(BM_TARGET),)
    BM_TARGET := all
  endif

  ifndef CUSTOM
    # Palabras posteriores al target (flags sin '=', p. ej. -flto)
    BM_CUSTOM_WORDS := $(wordlist 2, $(words $(BM_ARGS)), $(BM_ARGS))
    # Make interpreta "-march=native" como una variable llamada "-march" con valor "native"
    # (los argumentos con '=' nunca llegan a MAKECMDGOALS), así que se reconstruyen aquí.
    BM_CLI_FLAGS := $(foreach v,$(filter-out -*-%,$(filter -%,$(.VARIABLES))),$(v)=$($(v)))
    # Limpiamos las comillas residuales del string
    BM_CUSTOM := $(strip $(subst \",,$(subst \',,$(BM_CUSTOM_WORDS) $(BM_CLI_FLAGS))))
  else
    BM_CUSTOM := $(CUSTOM)
  endif

  ifeq ($(BM_TARGET),all)
    BM_LEVELS := $(OPT_LEVELS)
  else
    BM_LEVELS := $(BM_TARGET)
  endif
endif

#-------------

all: help

#----Compilación-------

build: $(MAIN)

run: $(MAIN) 
	./$(MAIN)

$(MAIN): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJECTS) -o $(MAIN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ------- Reglas de Testing -------
test: $(RUN_TEST_BINS)
	@echo "\033[1;34m--- Ejecutando Tests ---\033[0m"
	@fail=0; \
	for t in $(RUN_TEST_BINS); do \
		echo ">> $$t"; \
		./$$t || fail=1; \
		echo; \
	done; \
	if [ $$fail -eq 0 ]; then \
		echo "\033[1;32m--- Todos los tests OK ---\033[0m"; \
	else \
		echo "\033[1;31m--- Algún test falló ---\033[0m"; \
		exit 1; \
	fi
 
$(TEST_BIN_DIR)/%: $(TEST_BUILD_DIR)/%.o $(OBJECTS_WITHOUT_MAIN)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $< $(OBJECTS_WITHOUT_MAIN) -o $@
 
$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ------- Reglas de Benchmark -------

.PHONY: FORCE_BENCH

# Los contadores se cuentan dentro de las fuentes (macro COUNT_CALL de counter.h)
# compilando con -DBENCH_COUNT. Funciona igual en Linux, WSL, macOS y Windows:
# no usa --wrap (que no ve llamadas dentro del mismo .c) ni DYLD_INSERT_LIBRARIES.
# Por eso se compilan las fuentes directamente en vez de usar los .o normales.
BENCH_SOURCES = $(filter-out $(SRC_DIR)/main.c, $(SOURCES))

# En Linux/WSL, -std=c23 (estricto) oculta clock_gettime, fileno, mkdir, etc.: se piden las APIs POSIX.
# (macOS las expone sin esto; definirlo ahí ocultaría extensiones de Darwin.)
ifneq ($(UNAME_S),Darwin)
  BENCH_DEFS = -D_POSIX_C_SOURCE=200809L
endif

# $$t es la variable del bucle del shell (nivel de optimización)
BENCH_COMPILE = $(CC) $(CFLAGS) -O$$t -DBENCH_COUNT $(BENCH_DEFS) $(BM_CUSTOM) $(BENCH_DIR)/benchmark.c $(BENCH_DIR)/counter.c $(BENCH_SOURCES) -o $(BENCH_BIN_DIR)/bench_O$$t

# Un binario por nivel: se recompila siempre (benchmark es .PHONY)
benchmark:
	@mkdir -p $(BENCH_BIN_DIR)
	@for t in $(BM_LEVELS); do \
		echo "\n\033[1;33m--- Compilando con -O$$t $(BM_CUSTOM) ---\033[0m"; \
		echo "$(BENCH_COMPILE)"; \
		$(BENCH_COMPILE) || exit 1; \
		echo "\n\033[1;33m--- Ejecutando Benchmark ---\033[0m"; \
		echo ">> Target $$t | Flags: '$(BM_CUSTOM)' | Compilador: '$(CC_VERSION)'"; \
		$(BENCH_BIN_DIR)/bench_O$$t $$t "$(BM_CUSTOM)" "$(CC_VERSION)" || exit 1; \
	done

FORCE_BENCH:
# Objetivo vacío forzado para recompilar siempre el binario

#--------------

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

help:
	@echo "Uso: make [objetivo]"
	@echo
	@echo "Objetivos"
	@echo "    all        Muestra este mensaje"
	@echo "    run        Compila el proyecto y lo ejecuta"
	@echo "    test       Compila y ejecuta los tests"
	@echo "    benchmark  Ejecuta todos los benchmarks (equivalente a: benchmark all)"
	@echo "    clean      Elimina archivos generados"

KNOWN_COMMANDS = all build run test benchmark clean help FORCE_BENCH

.PHONY: $(KNOWN_COMMANDS)

# Regla atrapa-todo para suprimir errores de Make cuando pasas argumentos como "2" o "-O3"
ifeq (benchmark,$(firstword $(MAKECMDGOALS)))
%:
	@:
endif