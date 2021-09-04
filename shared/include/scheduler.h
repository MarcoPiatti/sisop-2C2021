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

/**
 * @DESC: Crea un objeto Process en memoria
 * @param id: id asignado al proceso
 * @return t_process*: puntero al Proceso
 */
t_process* createProcess(int id);

/**
 * @DESC: Destruye un Proceso de memoria
 * @param process: puntero al proceso a destruir
 */
void destroyProcess(t_process* process);

/**
 * @DESC: Crea una Cola de Procesos, protegida con mutex y semaforo
 * @return t_processQueue*: puntero a la cola protegida
 */
t_processQueue* createProcessQueue();

/**
 * @DESC: Destruye una cola de procesos
 * @param queue: puntero a la cola a destruir
 */
void destroyProcessQueue(t_processQueue* queue);

/**
 * @DESC: Mete un proceso en una cola de procesos
 * @param process: puntero al proceso
 * @param queue: puntero a la cola
 */
void putProcess(t_process* process, t_processQueue* queue);

/**
 * @DESC: Saca un puntero de una cola de procesos
 * @param queue: la cola
 * @return t_process*: puntero retornado
 */
t_process* takeProcess(t_processQueue* queue);

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

#endif // !LOGS_H_
