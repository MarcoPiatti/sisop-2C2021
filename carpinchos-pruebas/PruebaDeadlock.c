#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <matelib.h>

void* carpincho1_func(void* config){

    mate_instance instance;

    mate_init(&instance, config);

    printf("C1 - Toma SEM1\n");
    mate_sem_wait(&instance, "SEM1");
    sleep(3);
    printf("C1 - Toma SEM2\n");
    mate_sem_wait(&instance, "SEM2");
    sleep(3);

    printf("C1 - libera SEM1\n");
    mate_sem_post(&instance, "SEM1");
    printf("C1 - libera SEM2\n");
    mate_sem_post(&instance, "SEM2");

    printf("C2 - Se retira a descansar\n");
    mate_close(&instance);
    return 0;
}

void* carpincho2_func(void* config){

    mate_instance instance;

    mate_init(&instance, config);

    printf("C2 - toma SEM2\n");
    mate_sem_wait(&instance, "SEM2");
    sleep(3);
    printf("C2 - toma SEM3\n");
    mate_sem_wait(&instance, "SEM3");
    sleep(3);

    printf("C2 - libera SEM2\n");
    mate_sem_post(&instance, "SEM2");
    printf("C2 - libera SEM3\n");
    mate_sem_post(&instance, "SEM3");

	printf("C2 - Se retira a descansar\n");
	mate_close(&instance);
	return 0;
}

void* carpincho3_func(void* config){

    mate_instance instance;

    mate_init(&instance, config);

    printf("C3 - toma SEM3\n");
    mate_sem_wait(&instance, "SEM3");
    sleep(3);
    printf("C3 - toma SEM4\n");
    mate_sem_wait(&instance, "SEM4");
    sleep(3);

    printf("C3 - libera SEM3\n");
    mate_sem_post(&instance, "SEM3");
    printf("C3 - libera SEM4\n");
    mate_sem_post(&instance, "SEM4");

	printf("C3 - Se retira a descansar\n");
	mate_close(&instance);
	return 0;
}

void* carpincho4_func(void* config){

    mate_instance instance;

    mate_init(&instance, config);

    printf("C4 - toma SEM4\n");
    mate_sem_wait(&instance, "SEM4");
    sleep(3);
    printf("C4 - toma SEM1\n");
    mate_sem_wait(&instance, "SEM1");
    sleep(3);

    printf("C4 - libera SEM1\n");
    mate_sem_post(&instance, "SEM1");
    printf("C4 - libera SEM4\n");
    mate_sem_post(&instance, "SEM4");

	printf("C4 - Se retira a descansar\n");
	mate_close(&instance);
	return 0;
}

void* carpincho5_func(void* config){

    mate_instance instance;

    mate_init(&instance, config);

    printf("C5 - toma SEM5\n");
    mate_sem_wait(&instance, "SEM5");
    sleep(3);
    printf("C5 - toma SEM6\n");
    mate_sem_wait(&instance, "SEM6");
    sleep(3);

    printf("C5 - toma SEM5\n");
    mate_sem_post(&instance, "SEM5");
    printf("C5 - toma SEM6\n");
    mate_sem_post(&instance, "SEM6");

	printf("C5 - Se retira a descansar\n");
	mate_close(&instance);
	return 0;
}

void* carpincho6_func(void* config){

    mate_instance instance;

    mate_init(&instance, config);

    printf("C6 - toma SEM6\n");
    mate_sem_wait(&instance, "SEM6");
    sleep(3);
    printf("C6 - toma SEM1\n");
    mate_sem_wait(&instance, "SEM1");
    sleep(3);
    printf("C6 - Libera SEM1\n");
    mate_sem_post(&instance, "SEM1");

    printf("C6 - toma SEM5\n");
    mate_sem_wait(&instance, "SEM5");
    sleep(3);

    mate_sem_post(&instance, "SEM5");
    mate_sem_post(&instance, "SEM6");

	printf("C6 - Se retira a descansar\n");
	mate_close(&instance);
	return 0;
}

int main(int argc, char *argv[]) {

    mate_instance instance;

    mate_init(&instance, argv[1]);

  // Creamos los semaforos que van a usar los carpinchos
    mate_sem_init(&instance, "SEM1", 1);
    mate_sem_init(&instance, "SEM2", 1);
    mate_sem_init(&instance, "SEM3", 1);
    mate_sem_init(&instance, "SEM4", 1);
    mate_sem_init(&instance, "SEM5", 1);
    mate_sem_init(&instance, "SEM6", 1);

  // Deadlock entre estos 4
	pthread_t carpincho1;
	pthread_t carpincho2;
	pthread_t carpincho3;
	pthread_t carpincho4;

  // Deadlock entre estos 2 con uno pendiente del anterior
	pthread_t carpincho5;
	pthread_t carpincho6;


	printf("MAIN - Utilizando el archivo de config: %s\n", argv[1]);

	pthread_create(&carpincho1, NULL, carpincho1_func, argv[1]);
    sleep(1);
	pthread_create(&carpincho2, NULL, carpincho2_func, argv[1]);
    sleep(1);
	pthread_create(&carpincho3, NULL, carpincho3_func, argv[1]);
    sleep(1);
	pthread_create(&carpincho4, NULL, carpincho4_func, argv[1]);
    sleep(1);
	pthread_create(&carpincho5, NULL, carpincho5_func, argv[1]);
    sleep(1);
	pthread_create(&carpincho6, NULL, carpincho6_func, argv[1]);
    sleep(1);

    mate_close(&instance);

	pthread_join(carpincho6, NULL);
	pthread_join(carpincho5, NULL);
	pthread_join(carpincho4, NULL);
	pthread_join(carpincho3, NULL);
	pthread_join(carpincho2, NULL);
	pthread_join(carpincho1, NULL);
  
	printf("MAIN - Como no sabemos a quienes va a matar el algoritmo, entonces hacemos el free de los semáforos acá");
	mate_init(&instance, argv[1]);
    mate_sem_destroy(&instance, "SEM1");
    mate_sem_destroy(&instance, "SEM2");
    mate_sem_destroy(&instance, "SEM3");
    mate_sem_destroy(&instance, "SEM4");
    mate_sem_destroy(&instance, "SEM5");
    mate_sem_destroy(&instance, "SEM6");
	mate_close(&instance);

	printf("MAIN - Retirados los carpinchos de la pelea, hora de analizar los hechos\n");

	return EXIT_SUCCESS;
}
