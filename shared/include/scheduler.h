#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <commons/collections/queue.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_MULTIPROCESSING 10
#define QUANTUM 4
#define QUANTUM_LENGTH 1

typedef struct processQueue{
    t_queue* elems;
    pthread_mutex_t mutex;
    sem_t sem;
} t_processQueue;

typedef struct process{
    int pid;
    t_queue* tasks;
} t_process;

typedef struct task{
    bool isIO;
    int remaining;
} t_task;

t_processQueue *new, *ready, *blocked;

pthread_t thread_processInitializer, thread_executorIO, thread_executor[MAX_MULTIPROCESSING];

//TODO Probar que onda y poner declaraciones de funciones aca

#endif // !LOGS_H_
