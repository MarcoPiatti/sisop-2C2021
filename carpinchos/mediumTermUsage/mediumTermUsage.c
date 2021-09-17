#include <pthread.h>
#include "matelib.h"
#include <stdio.h>
#include <unistd.h>

#define CONFIG "./simpleMutex/simpleMutex.config"

//Ajustar config de kernel para que grado de multiprog < THREAD_COUNT

#define THREAD_COUNT 7
#define IO_DEVICE "laguna"

void *useIO(void* args){
    mate_instance mate;
    mate_init(&mate, CONFIG);
    mate_call_io(&mate, IO_DEVICE, NULL);
    mate_close(&mate);
}

int main(){
    pthread_t threads[THREAD_COUNT];
    
    for(int i = 0; i < THREAD_COUNT; i++){
        pthread_create(&threads[i], 0, useIO, NULL);
    }
    for(int i = 0; i < THREAD_COUNT; i++){
        pthread_join(threads[i], NULL);
    }
}