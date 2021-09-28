#include <pthread.h>
#include "matelib.h"
#include <stdio.h>
#include <unistd.h>

#define CONFIG "./memoryUsage/memoryUsage.config"

//Ajustar config de kernel para que grado de multiprog < THREAD_COUNT

int main(){
    mate_instance mate;
    mate_init(&mate, CONFIG);

    mate_pointer puntero = mate_memalloc(&mate, 1020);
    
    char* mensaje = "Hola :)";
    mate_memwrite(&mate, mensaje, puntero+900, 8);

    int algunValor;
    mate_memread(&mate, puntero+100, &algunValor, 4);

    mate_memfree(&mate, puntero);

    mate_close(&mate);
}