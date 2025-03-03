# Nombre del ejecutable
TARGET = thread

# Directorios
SRC_DIR = src
INCLUDE_DIR = $(SRC_DIR)/include
OBJ_DIR = obj
BIN_DIR = bin

# Archivos fuente y sus correspondientes archivos objeto
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Compilador
CC = gcc

# Flags comunes
CFLAGS = -Wall -Wextra -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function \
         -Wno-sign-conversion -Werror -fsanitize=undefined -std=gnu17

# Flags de depuración
DEBUG_FLAGS = -O0 -ggdb

# Flags de optimización
OPT_FLAGS = -O3 -fomit-frame-pointer -march=native -fno-strict-aliasing

# Incluye los archivos de cabecera
INCLUDES = -I$(INCLUDE_DIR)

# Elegir modo de compilación: DEBUG o OPT
ifdef DEBUG
    CFLAGS += $(DEBUG_FLAGS)
else
    CFLAGS += $(OPT_FLAGS)
endif

# Regla principal
all: build $(BIN_DIR)/$(TARGET)

# Crear directorios si no existen
build:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Regla para compilar el ejecutable
$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Regla para compilar archivos objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Regla para limpiar la compilación
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Regla para limpiar solo los binarios
clean-bin:
	rm -rf $(BIN_DIR)

# Regla para limpiar solo los objetos
clean-obj:
	rm -rf $(OBJ_DIR)

# Regla para recompilar todo
rebuild: clean all

.PHONY: all build clean clean-bin clean-obj rebuild
