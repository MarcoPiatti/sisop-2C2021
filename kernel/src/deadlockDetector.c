#include "deadlockDetector.h"
#include "networking.h"
#include <stdlib.h>

t_deadlockDetector* createDeadlockDetector(void*(*threadFunc)(void*)){
    t_deadlockDetector* tmp = malloc(sizeof(t_deadlockDetector));
    tmp->sems = NULL;
    tmp->m = 0;
    tmp->procs = NULL;
    tmp->n = 0;
    tmp->queue = pQueue_create();
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

    void destroyer(void* elem){
        destroyPacket((t_packet*)elem);
    };
    pQueue_destroy(dd->queue, destroyer);

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