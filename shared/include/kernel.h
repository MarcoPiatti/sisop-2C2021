#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <commons/collections/list.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_MULTIPROCESSING 10
#define QUANTUM 4
#define QUANTUM_LENGTH 1

typedef enum state { NEW, READY, EXEC, BLOCKED, SUSP_BLOCKED, SUSP_READY, MAX_STATES} t_state;

typedef struct processList{
    t_list* elems;
    pthread_mutex_t mutex;
    sem_t sem;
} t_processList;

typedef struct process{
    int pid;
    t_state state;
    int ultimaRafaga;
    int ultimaEstimacion;
    int nuevaEstimacion;
    int tiempoEsperado;
    int responseRatio;
} t_process; // Ponele

typedef struct IODevice{
    char* name;
    t_processList* blockedQueue;
}

t_processList* newQueue, readyQueue, susp_readyQueue;

pthread_t thread_Server; 
pthread_t thread_longTerm, thread_mediumTerm, thread_CPU[MAX_MULTIPROCESSING];

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
 * @return t_processList*: puntero a la cola protegida
 */
t_processList* createProcessQueue();

/**
 * @DESC: Destruye una cola de procesos
 * @param queue: puntero a la cola a destruir
 */
void destroyProcessQueue(t_processList* queue);

/**
 * @DESC: Mete un proceso en una cola de procesos
 * @param process: puntero al proceso
 * @param queue: puntero a la cola
 */
void putProcess(t_process* process, t_processList* queue);

/**
 * @DESC: Saca un puntero de una cola de procesos
 * @param queue: la cola
 * @return t_process*: puntero retornado
 */
t_process* takeProcess(t_processList* queue);

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
