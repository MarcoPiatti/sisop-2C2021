/*
 ============================================================================
 Name        : PruebaSuspension.c
 Description : Prueba de suspension de carpinchos del TP CarpinchOS
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <matelib.h>
#include <string.h>

#define SEMAFORO_SALUDO "SEM_HELLO"

int main(int argc, char *argv[]) {

    if(argc < 2){
        printf("No se ingresó archivo de configuración");
        exit(EXIT_FAILURE);
    }

    char* config = argv[1];

	printf("MAIN - Utilizando el archivo de config: %s\n", config);

	mate_instance instance;

	mate_init(&instance, (char*)config);

    char saludo[] = "¡Hola mundo!\n";

    mate_pointer saludoRef = mate_memalloc(&instance, strlen(saludo));

    mate_memwrite(&instance, saludo, saludoRef, strlen(saludo));

    mate_memread(&instance, saludoRef, saludo, strlen(saludo));

    printf(saludo);

    mate_sem_post(&instance, SEMAFORO_SALUDO);

    mate_close(&instance);

	return EXIT_SUCCESS;
}