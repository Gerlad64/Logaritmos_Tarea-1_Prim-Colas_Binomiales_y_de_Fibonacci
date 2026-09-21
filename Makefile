
# compilador
CC = gcc
#flags del compilador
CFLAGS = -I./include -std=c23

# Definir directorios
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

#Ejecutables
MAIN = $(BIN_DIR)/main

# Archivos fuente (.c) y objetos (.o)
SOURCES = $(wildcard $(SRC_DIR)/*.c)
# ejemplo: main.c --> main.o se guarda en build
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# --- Variables para Testing ---
TEST_DIR = tests
TEST_BUILD_DIR = $(BUILD_DIR)/tests
TEST_BIN_DIR = $(BIN_DIR)/tests
 
# Cada archivo de test tiene su propio main(), así que cada uno
# se compila y enlaza como un ejecutable INDEPENDIENTE.
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(TEST_BUILD_DIR)/%.o)
TEST_BINS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(TEST_BIN_DIR)/%)
 
# Filtramos main.o de los objetos del proyecto para que no choque con el main de los tests
# (Asumiendo que tu archivo principal se llama exactamente src/main.c)
OBJECTS_WITHOUT_MAIN = $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS))

#-------------

# Regla all (solo ejecutar make)
# Lanza mensaje de ayuda
all: help


#----Compilación-------

# Regla make build
# Compila el proyecto
build: $(MAIN)

#Regla make run
# Compila (si no estaba compilado) 
# y ejecuta el proyecto
run: $(MAIN) 
	./$(MAIN)

$(MAIN): $(OBJECTS) # Depende de OBJECTS
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJECTS) -o $(MAIN)

#Compilación de objetos, se guardan en build/
# Esto no lo entiendo mucho pero funciona
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ------- Reglas de Testing -------
 
# Regla make test: Compila y ejecuta CADA test por separado
# (cada archivo en tests/ tiene su propio main(), por lo que no
# se pueden enlazar todos juntos en un solo ejecutable)
test: $(TEST_BINS)
	@echo "\033[1;34m--- Ejecutando Tests ---\033[0m"
	@fail=0; \
	for t in $(TEST_BINS); do \
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
 
# Enlaza cada test como ejecutable independiente
# (usa OBJECTS_WITHOUT_MAIN para no chocar con el main() del proyecto)
$(TEST_BIN_DIR)/%: $(TEST_BUILD_DIR)/%.o $(OBJECTS_WITHOUT_MAIN)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $< $(OBJECTS_WITHOUT_MAIN) -o $@
 
# Compilación de los objetos de test (en su propio subdirectorio
# para no chocar con la regla patrón de $(BUILD_DIR)/%.o de src/)
$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
#--------------

# Limpieza de archivos generados
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)


help:
	@echo "Uso: make [objetivo]"
	@echo
	@echo "Objetivos"
	@echo "    all  	Muestra este mensaje"
	@echo "    run  	Compila el proyecto y lo ejecuta"
	@echo "    test  	Compila y ejecuta los tests"
	@echo "    clean 	Elimina archivos generados"
	@echo "    help 	Muestra este mensaje"

KNOWN_COMMANDS = all build run clean help

.PHONY: $(KNOWN_COMMANDS)
