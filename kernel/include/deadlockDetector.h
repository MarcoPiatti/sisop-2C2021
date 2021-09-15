#include "process.h"
#include "mateSem.h"
#include "pQueue.h"
#include <pthread.h>

typedef enum DDMsg { DD_SEM_INIT, DD_SEM_ALLOC_INST, DD_SEM_ALLOC, DD_SEM_REQ, DD_SEM_REL, DD_SEM_DESTROY, DD_PROC_INIT, DD_PROC_TERM, DD_MAX } DDMsg;

typedef struct deadlockDetector {
    t_mateSem* *sems;
    int m;
    t_process* *procs;
    int n;
    int *available;
    int **allocation;
    int **request;
    t_pQueue* queue;
    pthread_t thread;
} t_deadlockDetector;

t_deadlockDetector* createDeadlockDetector(void*(*threadFunc)(void*));

void destroyDeadlockDetector(t_deadlockDetector* dd);