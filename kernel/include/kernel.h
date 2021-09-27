#ifndef  KERNEL_H_
#define  KERNEL_H_

#include "pQueue.h"
#include "process.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_MULTIPROCESSING 10

//Colas de estado compartidas
t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedReadyQueue;

pthread_t thread_longTerm, thread_mediumTerm, thread_shortTerm;

/**
 * @DESC: Funcion de thread que lleva procesos de new a ready
 * @param nada: obligatorio por firma, no se usa
 * @return void*: obligatorio por firma, no se usa
 */
void* processInitializer(void* nada);

/**
 * @DESC: Ejecuta las tareas de un proceso, determinados quantums
 * @param tasks: lista de tareas de un proceso
 * @param quantums: cantidad de "ciclos de cpu" que se ejecutan, si es 0 es indeterminado
 * @return int: 
 * - Retorna -1 si no hay mas tareas para procesar.
 * - Retorna 0 si la tarea a procesar es IO.
 * - Retorna 1 si quedan mas tareas CPU por procesar.
 */
int runCPU(t_queue* tasks, int quantums);

/**
 * @DESC: Funcion de thread que ejecuta rafagas de CPU de procesos
 * @param nada: obligatorio por firma, no se usa
 * @return void*: obligatorio por firma, no se usa
 */
void* executor(void* nada);

/**
 * @DESC: Ejecuta las tareas IO de un proceso, hasta llegar a una de CPU
 * @param tasks: cola de tareas de un proceso
 */
void runIO(t_queue* tasks);

/**
 * @DESC: Funcion de thread que ejecuta las rafagas IO de procesos
 * @param nada: obligatorio por firma, no se usa
 * @return void*: obligatorio por firma, no se usa
 */
void* executorIO(void* nada);

/**
 * @DESC: Crea todas las estructuras administrativas e hilos
 *        Para que empiece a funcionar el scheduler
 */
void createScheduler();

void *auxHandler(void *vclientSocket);

#endif // !KERNEL_H_
