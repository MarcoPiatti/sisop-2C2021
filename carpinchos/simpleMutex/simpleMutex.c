#include <pthread.h>
#include "matelib.h"
#include <stdio.h>
#include <unistd.h>

static int count = 10;
#define CONFIG "./simpleMutex/simpleMutex.config"

void *thread1(void* args){
    mate_instance mate;
    mate_init(&mate, CONFIG);
    printf("me comunique con el server\n");
    for(int i=0; i < 2; i++){
        mate_sem_wait(&mate, "semCarpinchito");
        count++;
        mate_sem_post(&mate, "semCarpinchito");
    }
    mate_close(&mate);
    printf("se fue thread1\n");
}

void *thread2(void* args){
    mate_instance mate;
    mate_init(&mate, CONFIG);
    printf("me comunique con el server\n");
    for(int i=0; i < 2; i++){
        mate_sem_wait(&mate, "semCarpinchito");
        count--;
        mate_sem_post(&mate, "semCarpinchito");
    }
    mate_close(&mate);
    printf("se fue thread2\n");
}

int main(){
    pthread_t unThread, otroThread;
    
    mate_instance mate;
    mate_init(&mate, CONFIG);
    if(mate_sem_init(&mate, "semCarpinchito", 1) == 0);

    pthread_create(&unThread, 0, thread1, NULL);
    pthread_create(&otroThread, 0, thread2, NULL);
    pthread_join(unThread, NULL);
    pthread_join(otroThread, NULL);

    printf("%i\n", count);
    mate_sem_destroy(&mate, "semCarpinchito");
    mate_close(&mate);
    printf("se fue el main\n");
}