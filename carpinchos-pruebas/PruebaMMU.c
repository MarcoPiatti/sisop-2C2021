/*
 ============================================================================
 Name        : PruebaMMU.c
 Description : Prueba de MMU del TP CarpinchOS
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <matelib.h>

sem_t semCarpincho1;
sem_t semCarpincho2;
sem_t semCarpincho3;

void* carpincho1_func(void* config){

	sem_wait(&semCarpincho1);
	mate_instance instanceC1;

	printf("C1 - Llamo a mate_init\n");
	mate_init(&instanceC1, (char*)config);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc0 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Libero al C2\n");
	sem_post(&semCarpincho2);

	printf("C1 - Freno a C1\n");
	sem_wait(&semCarpincho1);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc1 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Libero al C2\n");
	sem_post(&semCarpincho2);

	printf("C1 - Freno a C1\n");
	sem_wait(&semCarpincho1);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc2 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Libero al C2\n");
	sem_post(&semCarpincho2);

	printf("C1 - Freno a C1\n");
	sem_wait(&semCarpincho1);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc3 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc4 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc5 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc6 = mate_memalloc(&instanceC1, 23);

	printf("C1 - Escribo en la página 0\n");
	mate_memwrite(&instanceC1, "Hola", alloc0, 5);

	printf("C1 - Escribo en la página 1\n");
	mate_memwrite(&instanceC1, "Chau", alloc1, 5);

	printf("C1 - Libero al C2\n");
	sem_post(&semCarpincho2);

	printf("C1 - Freno a C1\n");
	sem_wait(&semCarpincho1);

	printf("C1 - Escribo en la página 2\n");
	mate_memwrite(&instanceC1, "Carpincho", alloc2, 10);

	printf("C1 - Escribo en la página 3\n");
	mate_memwrite(&instanceC1, "Capibara", alloc3, 9);

	printf("C1 - Escribo en la página 4\n");
	mate_memwrite(&instanceC1, "Hydrochaeris", alloc4, 13);

	printf("C1 - Reservo un alloc de 21 bytes\n");
	mate_pointer alloc7 = mate_memalloc(&instanceC1, 21);

	printf("C1 - Libero al C2 para que finalice\n");
	sem_post(&semCarpincho2);
	
	printf("C1 - Libero al C3 para que finalice\n");
	sem_post(&semCarpincho3);

	printf("C1 - Se retira a descansar\n");
	mate_close(&instanceC1);

	return 0;
}

void* carpincho2_func(void* config){

	sem_wait(&semCarpincho2);
	mate_instance instanceC2;

	printf("C2 - Llamo a mate_init\n");
	mate_init(&instanceC2, (char*)config);

	printf("C2 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc0 = mate_memalloc(&instanceC2, 23);

	printf("C2 - Libero al C3\n");
	sem_post(&semCarpincho3);

	printf("C2 - Freno a C2\n");
	sem_wait(&semCarpincho2);

	printf("C2 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc1 = mate_memalloc(&instanceC2, 23);

	printf("C2 - Libero al C3\n");
	sem_post(&semCarpincho3);

	printf("C2 - Freno a C2\n");
	sem_wait(&semCarpincho2);

	printf("C2 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc2 = mate_memalloc(&instanceC2, 23);

	printf("C2 - Libero al C3\n");
	sem_post(&semCarpincho3);

	printf("C2 - Freno a C2\n");
	sem_wait(&semCarpincho2);

	void* localMalloc = malloc(5);

	printf("C2 - Leo de la página 0\n");
	mate_memread(&instanceC2, alloc0, localMalloc , 5);

	printf("C2 - Leo de la página 1\n");
	mate_memread(&instanceC2, alloc1, localMalloc, 5);

	printf("C2 - Libero al C3\n");
	sem_post(&semCarpincho3);

	printf("C2 - Freno a C2\n");
	sem_wait(&semCarpincho2);

	printf("C2 - Se retira a descansar\n");
	mate_close(&instanceC2);

	return 0;
}

void* carpincho3_func(void* config){

	sem_wait(&semCarpincho3);
	mate_instance instanceC3;

	printf("C3 - Llamo a mate_init\n");
	mate_init(&instanceC3, (char*)config);

	printf("C3 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc0 = mate_memalloc(&instanceC3, 23);

	printf("C3 - Libero al C1\n");
	sem_post(&semCarpincho1);

	printf("C3 - Freno a C3\n");
	sem_wait(&semCarpincho3);

	printf("C3 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc1 = mate_memalloc(&instanceC3, 23);

	printf("C3 - Libero al C1\n");
	sem_post(&semCarpincho1);

	printf("C3 - Freno a C3\n");
	sem_wait(&semCarpincho3);

	printf("C3 - Reservo un alloc de 23 bytes\n");
	mate_pointer alloc2 = mate_memalloc(&instanceC3, 23);

	printf("C3 - Libero al C1\n");
	sem_post(&semCarpincho1);

	printf("C3 - Freno a C3\n");
	sem_wait(&semCarpincho3);

	void* localMalloc = malloc(5);

	printf("C3 - Leo de la página 0\n");
	mate_memread(&instanceC3, alloc0, localMalloc , 5);

	printf("C3 - Leo de la página 1\n");
	mate_memread(&instanceC3, alloc1, localMalloc, 5);

	printf("C3 - Libero al C1\n");
	sem_post(&semCarpincho1);

	printf("C3 - Freno a C3\n");
	sem_wait(&semCarpincho3);

	printf("C3 - Se retira a descansar\n");
	mate_close(&instanceC3);

	return 0;
}



int main(int argc, char *argv[]) {

	pthread_t carpincho1;
	pthread_t carpincho2;
	pthread_t carpincho3;

	sem_init(&semCarpincho1, 0, 1);
	sem_init(&semCarpincho2, 0, 0);
	sem_init(&semCarpincho3, 0, 0);

	printf("MAIN - Utilizando el archivo de config: %s\n", argv[1]);

	pthread_create(&carpincho1, NULL, carpincho1_func, argv[1]);
	pthread_create(&carpincho2, NULL, carpincho2_func, argv[1]);
	pthread_create(&carpincho3, NULL, carpincho3_func, argv[1]);

	pthread_join(carpincho3, NULL);
	pthread_join(carpincho2, NULL);
	pthread_join(carpincho1, NULL);

	printf("MAIN - Retirados los carpinchos de la pelea, hora de analizar los hechos\n");

	return EXIT_SUCCESS;
}
