#include "process.h"
#include "mateSem.h"
#include <pthread.h>

/**
 * @DESC: Informacion que un Deadlock detector necesita
 * - sems: array dinamico con referencias a los semaforos del sistema
 * - m: cantidad de semaforos
 * - procs: array dinamico con referencias a todos los procesos del sistema
 * - n: cantidad de procesos
 * - available: array con cantidad de recursos disponibles de cada semaforo
 * - allocation: matriz con cantidad de recursos de cada semaforo que tiene cada proceso
 * - request: matriz con cantidad de recursos de cada semaforo que pide cada proceso
 * - mutex: mutex para acceso exclusivo a las estructuras del detector
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
    pthread_mutex_t mutex;
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

/**
 * @DESC: Cuando se crea un semaforo nuevo, el DD lo agrega a sus matrices
 */
void DDSemInit(t_deadlockDetector* dd, t_mateSem* newSem, int newSemValue);

/**
 * @DESC: Cuando un proceso pasa derecho por un sem, el DD actualiza sus matrices
 */
void DDSemAllocatedInstant(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem);

/**
 * @DESC: Cuando un proceso paso por un sem despues de esperar, el DD actualiza sus matrices
 */
void DDSemAllocated(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem);

/**
 * @DESC: Cuando un proceso se freno en espera de un semaforo, el DD actualiza sus matrices
 */
void DDSemRequested(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem);

/**
 * @DESC: Cuando un proceso postea un semaforo, el DD actualiza sus matrices
 */
void DDSemRelease(t_deadlockDetector* dd, t_process* proc, t_mateSem* sem);

/**
 * @DESC: Cuando un semaforo es destruido, el DD reactualiza sus matrices
 */
void DDSemDestroy(t_deadlockDetector* dd, t_mateSem* sem);

/**
 * @DESC: Cuando un proceso es iniciado, el DD actualiza sus matrices
 */
void DDProcInit(t_deadlockDetector* dd, t_process* newProc);

/**
 * @DESC: Cuando un proceso es terminado, el DD reactualiza sus matrices
 */
void DDProcTerm(t_deadlockDetector* dd, t_process* proc);