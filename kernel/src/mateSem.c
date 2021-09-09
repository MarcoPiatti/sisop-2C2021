#include "mateSem.h"

#include "commons/string.h"
#include <stdlib.h>

t_mateSem hola;
int hola;


t_mateSem* mateSem_create(char* nombre, unsigned int contadorInicial, void* (* mateSemFunc)(void*)){
    t_mateSem* mateSem = malloc(sizeof(t_mateSem));
    mateSem->nombre = string_duplicate(nombre);
    sem_init(&mateSem->sem, 0, contadorInicial);
    mateSem->waitingProcesses = pQueue_create();
    pthread_create(&mateSem->thread_mateSem, 0, mateSemFunc, (void*)mateSem);
    pthread_detach(mateSem->thread_mateSem);
    return mateSem;
}

void mateSem_destroy(t_mateSem* mateSem){
    pthread_cancel(mateSem->thread_mateSem);
    pQueue_destroy(mateSem->waitingProcesses, destroyProcess);
    free(mateSem->nombre);
    free(mateSem);
}

void mateSem_wait(t_mateSem* mateSem, t_process* process){
    pQueue_put(mateSem->waitingProcesses, (void*)process);
}

void mateSem_post(t_mateSem* mateSem){
    sem_post(&mateSem->sem);
}