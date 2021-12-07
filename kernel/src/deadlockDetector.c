#include "deadlockDetector.h"
#include "networking.h"
#include <stdlib.h>

t_deadlockDetector* createDeadlockDetector(void*(*threadFunc)(void*)){
    t_deadlockDetector* tmp = malloc(sizeof(t_deadlockDetector));
    tmp->sems = NULL;
    tmp->m = 0;
    tmp->procs = NULL;
    tmp->n = 0;
    pthread_mutex_init(&tmp->mutex, NULL);
    tmp->available = NULL;
    tmp->allocation = NULL;
    tmp->request = NULL;
    pthread_create(&tmp->thread, 0, threadFunc, (void*)tmp);
    return tmp;
}

void destroyDeadlockDetector(t_deadlockDetector* dd){
    pthread_cancel(dd->thread);
    pthread_join(dd->thread, NULL);

    free(dd->sems);
    free(dd->procs);

    pthread_mutex_destroy(&dd->mutex);

    free(dd->available);

    for(int i = 0; i < dd->n; i++){
        free(dd->allocation[i]);
    }
    free(dd->allocation);

    for(int i = 0; i < dd->n; i++){
        free(dd->request[i]);
    }
    free(dd->request);
}

/**
 * @DESC: Cuando se crea un semaforo nuevo, el DD lo agrega a sus matrices
 */
void DDSemInit(t_deadlockDetector* dd, t_mateSem* newSem, int newSemValue){
    pthread_mutex_lock(&dd->mutex);

    dd->m++;
    dd->sems = realloc(dd->sems, sizeof(t_mateSem*) * dd->m);
    dd->sems[dd->m-1] = newSem;

    dd->available = realloc(dd->available, sizeof(int) * dd->m);
    dd->available[dd->m-1] = newSemValue;
    
    for(int i = 0; i < dd->n; i++){
        dd->allocation[i] = realloc(dd->allocation[i], sizeof(int) * dd->m);
        dd->allocation[i][dd->m-1] = 0;
        dd->request[i] = realloc(dd->request[i], sizeof(int) * dd->m);
        dd->request[i][dd->m-1] = 0;
    }

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un proceso pasa derecho por un sem, el DD actualiza sus matrices
 */
void DDSemAllocatedInstant(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem){
    pthread_mutex_lock(&dd->mutex);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->available[j]--;
    dd->allocation[i][j]++;

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un proceso paso por un sem despues de esperar, el DD actualiza sus matrices
 */
void DDSemAllocated(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem){
    pthread_mutex_lock(&dd->mutex);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->available[j]--;
    dd->request[i][j]--;
    dd->allocation[i][j]++;

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un proceso se freno en espera de un semaforo, el DD actualiza sus matrices
 */
void DDSemRequested(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem){
    pthread_mutex_lock(&dd->mutex);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->request[i][j]++;

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un proceso postea un semaforo, el DD actualiza sus matrices
 */
void DDSemRelease(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem){
    pthread_mutex_lock(&dd->mutex);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->available[j]++;
    if(dd->allocation[i][j])dd->allocation[i][j]--;

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un semaforo es destruido, el DD reactualiza sus matrices
 */
void DDSemDestroy(t_deadlockDetector* dd, t_mateSem* sem){
    pthread_mutex_lock(&dd->mutex);

    dd->m--;
    int j = 0;
    while (dd->sems[j] != sem) j++;

    memmove(dd->sems+j, dd->sems+j+1, sizeof(t_mateSem*) * (dd->m - j));
    dd->sems = realloc(dd->sems, sizeof(t_mateSem*) * dd->m);

    memmove(dd->available+j, dd->available+j+1, sizeof(int) * (dd->m - j));
    dd->available = realloc(dd->available, sizeof(int) * dd->m);
    
    for(int i = 0; i < dd->n; i++){
        memmove(dd->allocation[i]+j, dd->allocation[i]+j+1, sizeof(int) * (dd->m - j));
        dd->allocation[i] = realloc(dd->allocation[i], sizeof(int) * dd->m);

        memmove(dd->request[i]+j, dd->request[i]+j+1, sizeof(int) * (dd->m - j));
        dd->request[i] = realloc(dd->request[i], sizeof(int) * dd->m);
    }

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un proceso es iniciado, el DD actualiza sus matrices
 */
void DDProcInit(t_deadlockDetector* dd, t_process* newProc){
    pthread_mutex_lock(&dd->mutex);

    dd->n++;
    dd->procs = realloc(dd->procs, sizeof(t_process*) * dd->n);
    dd->procs[dd->n-1] = newProc;

    dd->allocation = realloc(dd->allocation, sizeof(int*) * dd->n);
    dd->request = realloc(dd->request, sizeof(int*) * dd->n);

    if(dd->m!=0){
        dd->allocation[dd->n-1] = calloc(1, sizeof(int) * dd->m);
        dd->request[dd->n-1] = calloc(1, sizeof(int) * dd->m);
    }
    else{
        dd->allocation[dd->n-1] = NULL;
        dd->request[dd->n-1] = NULL;
    }

    pthread_mutex_unlock(&dd->mutex);
}

/**
 * @DESC: Cuando un proceso es terminado, el DD reactualiza sus matrices
 */
void DDProcTerm(t_deadlockDetector* dd, t_process* proc){
    pthread_mutex_lock(&dd->mutex);

    dd->n--;
    int i = 0;
    while (dd->procs[i] != proc) i++;

    memmove(dd->procs+i, dd->procs+i+1, sizeof(t_process*) * (dd->n - i));
    dd->procs = realloc(dd->procs, sizeof(t_process*) * dd->n);

    free(dd->allocation[i]);
    memmove(dd->allocation+i, dd->allocation+i+1, sizeof(int*) * (dd->n - i));
    dd->allocation = realloc(dd->allocation, sizeof(int*) * dd->n);

    free(dd->request[i]);
    memmove(dd->request+i, dd->request+i+1, sizeof(int*) * (dd->n - i));
    dd->request = realloc(dd->request, sizeof(int*) * dd->n);

    pthread_mutex_unlock(&dd->mutex);
}