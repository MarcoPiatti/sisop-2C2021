#include "process.h"
#include "mateSem.h"
#include "pQueue.h"
#include <pthread.h>

/**
 * @DESC: Mensajes enviados al Deadlock detector por los otros hilos del kernel
 */
typedef enum DDMsg { DD_SEM_INIT, DD_SEM_ALLOC_INST, DD_SEM_ALLOC, DD_SEM_REQ, DD_SEM_REL, DD_SEM_DESTROY, DD_PROC_INIT, DD_PROC_TERM, DD_MAX } DDMsg;

/**
 * @DESC: Informacion que un Deadlock detector necesita
 * - sems: array dinamico con referencias a los semaforos del sistema
 * - m: cantidad de semaforos
 * - procs: array dinamico con referencias a todos los procesos del sistema
 * - n: cantidad de procesos
 * - available: array con cantidad de recursos disponibles de cada semaforo
 * - allocation: matriz con cantidad de recursos de cada semaforo que tiene cada proceso
 * - request: matriz con cantidad de recursos de cada semaforo que pide cada proceso
 * - queue: cola de mensajes que llegan avisando de cambios en el sistema
 * - thread: hilo que ejecuta haciendo de deadlock detector
 */
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

/**
 * @DESC: Crea un objeto Deadlock Detector en memoria, a partir de una funcion provista para el hilo
 */
t_deadlockDetector* createDeadlockDetector(void*(*threadFunc)(void*));

/**
 * @DESC: Destruye un Deadlock detector
 */
void destroyDeadlockDetector(t_deadlockDetector* dd);