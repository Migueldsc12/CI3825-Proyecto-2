# Makefile para teopp (procesos)
CC = gcc

DEBUG_FLAGS = -Wall -Wextra -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -std=gnu17 -Werror -O0 -ggdb
RELEASE_FLAGS = -Wall -Wextra -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -Werror -fsanitize=undefined -std=gnu17 -O3 -fomit-frame-pointer -march=native -fno-strict-aliasing

# Por defecto, modo debug; para release invoca: make MODE=release
MODE ?= debug

ifeq ($(MODE), debug)
    CFLAGS = $(DEBUG_FLAGS)
else ifeq ($(MODE), release)
    CFLAGS = $(RELEASE_FLAGS)
else
    $(error Modo desconocido $(MODE). Use "debug" o "release")
endif

TARGET = teopp
SOURCES = teopp.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
