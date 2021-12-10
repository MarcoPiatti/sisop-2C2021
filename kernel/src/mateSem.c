#include "mateSem.h"

#include "commons/string.h"
#include <stdlib.h>

t_mateSem* mateSem_create(char* nombre, unsigned int contadorInicial, void* (* mateSemFunc)(void*)){
    t_mateSem* mateSem = malloc(sizeof(t_mateSem));
    mateSem->nombre = string_duplicate(nombre);
    mateSem->sem = contadorInicial;
    pthread_cond_init(&mateSem->sem_cond, NULL);
    pthread_mutex_init(&mateSem->sem_mutex, NULL);
    mateSem->waitingProcesses = pQueue_create();
    pthread_create(&mateSem->thread_mateSem, 0, mateSemFunc, (void*)mateSem);
    return mateSem;
}

void mateSem_destroy(t_mateSem* mateSem){
    pthread_cancel(mateSem->thread_mateSem);
    pthread_join(mateSem->thread_mateSem, NULL);
    void destroyer(void*elem){
        destroyProcess((t_process*)elem);
    };
    pQueue_destroy(mateSem->waitingProcesses, destroyer);
    free(mateSem->nombre);
    pthread_cond_destroy(&mateSem->sem_cond);
    pthread_mutex_destroy(&mateSem->sem_mutex);
    free(mateSem);
}

bool mateSem_wait(t_mateSem* mateSem, t_process* process){
    bool rc;
    pthread_mutex_lock(&mateSem->sem_mutex);
    if(mateSem->sem < 1){
        pQueue_put(mateSem->waitingProcesses, (void*)process);
        rc = true;
    }
    else{
        mateSem->sem--;
        rc = false;
    }
    pthread_mutex_unlock(&mateSem->sem_mutex);
    return rc;
}

void mateSem_post(t_mateSem* mateSem){
    pthread_mutex_lock(&mateSem->sem_mutex);
    if(!pQueue_isEmpty(mateSem->waitingProcesses) && !mateSem->sem){
        pthread_cond_signal(&mateSem->sem_cond);
    }
    mateSem->sem++;
    pthread_mutex_unlock(&mateSem->sem_mutex);
}