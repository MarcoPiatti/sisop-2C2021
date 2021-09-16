#include <pthread.h>
#include "matelib.h"
#include <stdio.h>
#include <unistd.h>

static int count = 10;
#define CONFIG "./simpleDeadlock/simpleDeadlock.config"

int retcode1;
int retcode2;

void *thread1(void* args){
    mate_instance mate;
    mate_init(&mate, CONFIG);
        if(mate_sem_wait(&mate, "SEM1")) pthread_exit(NULL);
        if(mate_sem_wait(&mate, "SEM2")) pthread_exit(NULL);
        count++;
        mate_sem_post(&mate, "SEM2");
        mate_sem_post(&mate, "SEM1");
    mate_close(&mate);
    printf("se fue thread1\n");
    pthread_exit(1);
}

void *thread2(void* args){
    mate_instance mate;
    mate_init(&mate, CONFIG);
        if(mate_sem_wait(&mate, "SEM2")) pthread_exit(NULL);
        if(mate_sem_wait(&mate, "SEM1")) pthread_exit(NULL);
        count++;
        mate_sem_post(&mate, "SEM1");
        mate_sem_post(&mate, "SEM2");
    mate_close(&mate);
    printf("se fue thread2\n");
    pthread_exit(1);
}

int main(){
    pthread_t unThread, otroThread;
    
    mate_instance mate;
    mate_init(&mate, CONFIG);
    mate_sem_init(&mate, "SEM1", 1);
    mate_sem_init(&mate, "SEM2", 1);

    pthread_create(&unThread, 0, thread1, NULL);
    pthread_create(&otroThread, 0, thread2, NULL);
    pthread_join(unThread, &retcode1);
    pthread_join(otroThread, &retcode2);

    if(retcode1) printf("el hilo 1 salio bien\n");
    if(!retcode1) printf("el hilo 1 salio mal\n");
    if(retcode2) printf("el hilo 2 salio bien\n");
    if(!retcode2) printf("el hilo 2 salio mal\n");
    mate_sem_destroy(&mate, "SEM1");
    mate_sem_destroy(&mate, "SEM2");
    mate_close(&mate);
    printf("se fue el main\n");
}