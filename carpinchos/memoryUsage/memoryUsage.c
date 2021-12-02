#include <pthread.h>
#include "matelib.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define CONFIG "./memoryUsage/memoryUsage.config"

// Carpincho para asertar integridad de datos en alloc, write, read, free
// Se le manda la cadena "Hola :)" y se la lee con otra variable para contrastar que sean iguales.

int main(){
    mate_instance mate;
    mate_init(&mate, CONFIG);

    mate_pointer puntero = mate_memalloc(&mate, 200);

    char* mensaje = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    printf("Mensaje a escribir: %s.\n", mensaje);
    mate_memwrite(&mate, mensaje, puntero, 200);

    char* mensajeLeido = malloc(200);
    mate_memread(&mate, puntero, mensajeLeido, 200);
    // mate_memfree(&mate, puntero);
    mate_close(&mate);
    printf("Mensaje leido: %s\n", mensajeLeido);
}