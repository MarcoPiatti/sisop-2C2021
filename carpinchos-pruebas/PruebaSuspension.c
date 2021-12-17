/*
 ============================================================================
 Name        : PruebaSuspension.c
 Description : Prueba de suspension de carpinchos del TP CarpinchOS
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <matelib.h>


void* carpincho1_func(void* config){

	mate_instance instance;

	printf("C1 - Llamo a mate_init\n");
	mate_init(&instance, (char*)config);

	printf("C1 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C1 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C1 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

    mate_close(&instance);

	return 0;
}

void* carpincho2_func(void* config){

	mate_instance instance;

	printf("C2 - Llamo a mate_init\n");
	mate_init(&instance, (char*)config);

	printf("C2 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C2 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C2 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

    mate_close(&instance);

	return 0;
}

void* carpincho3_func(void* config){

	mate_instance instance;

	printf("C3 - Llamo a mate_init\n");
	mate_init(&instance, (char*)config);

	printf("C3 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C3 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C3 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

    mate_close(&instance);

	return 0;
}

void* carpincho4_func(void* config){

	mate_instance instance;

	printf("C4 - Llamo a mate_init\n");
	mate_init(&instance, (char*)config);

	printf("C4 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C4 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C4 - Hace una llamada a IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

    mate_close(&instance);

	return 0;
}



int main(int argc, char *argv[]) {

	pthread_t carpincho1;
	pthread_t carpincho2;
	pthread_t carpincho3;
	pthread_t carpincho4;

	printf("MAIN - Utilizando el archivo de config: %s\n", argv[1]);

	pthread_create(&carpincho1, NULL, carpincho1_func, argv[1]);
	sleep(1);
	pthread_create(&carpincho2, NULL, carpincho2_func, argv[1]);
	sleep(1);
	pthread_create(&carpincho3, NULL, carpincho3_func, argv[1]);
	sleep(1);
	pthread_create(&carpincho4, NULL, carpincho4_func, argv[1]);

	pthread_join(carpincho4, NULL);
	pthread_join(carpincho3, NULL);
	pthread_join(carpincho2, NULL);
	pthread_join(carpincho1, NULL);

	printf("MAIN - Retirados los carpinchos de la pelea, hora de analizar los hechos\n");

	return EXIT_SUCCESS;
}
