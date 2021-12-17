################################################################################
# Makefile para compilar las pruebas del tp CarpinchOS
################################################################################

# Agrego las libs
LIBS := -lmatelib -lpthread -lcommons

# Agrego los archivos a compilar (todos los archivos *.c en esta carpeta)
CAPY_SRCS := $(wildcard *.c)

# Agrego los nombres de los carpinchos (que van a estar en una carpeta "build")
CARPINCHOS := $(CAPY_SRCS:%.c=build/%)

# Agrego los headers de matelib
HEADERS := -I. -I./lib

# Agrego comandos extra
RM := rm -rf
CC := gcc

# compile --> Crea la carpeta "build" y compila todos los carpinchos
compile: build $(CARPINCHOS)

build:
	mkdir $@

build/%: %.c
	$(CC) $(HEADERS) -o "$@" $^ $(LIBS)

# clean --> Limpia todos los carpinchos
clean:
	-$(RM) build

.PHONY:
	compile clean