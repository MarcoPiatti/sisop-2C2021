#ifndef MATESEM_H_
#define MATESEM_H_

#include <pthread.h>
#include "pQueue.h"
#include "process.h"

//Todo: actualizar comentarios de documentacion

/**
 * @DESC: Semaforo para carpinchos.
 *        tiene un nombre, y un semaforo interno con el cual gestiona su contador.
 *        tiene una cola de procesos en espera (que hicieron mateSem_wait).
 *        tiene un thread que con cada post debe tomar un proceso de la cola y lo pone en ready
 */
typedef struct mateSem {
    char* nombre;
    int semaforo;
    t_pQueue* waitingProcesses;
} t_mateSem;

/**
 * @DESC: Crea un mateSem en memoria
 * @param nombre: nombre del mateSem
 * @param contadorInicial: valor inicial del contador.
 * @param mateSemFunc: funcion que ejecutara el thread del mateSem.
 *                     Debe tomar por argumento un puntero a mateSem, para tener una referencia de si mismo.
 *                     Debe hacer un wait del semaforo interno cada vez que tome un proceso.
 *                     Y luego meterlo a una cola de ready, o similar.
 * @return t_mateSem*: puntero al struct del mateSem creado
 */
t_mateSem* mateSem_create(char* nombre, unsigned int contadorInicial);

/**
 * @DESC: Destruye un mateSem, su thread, su cola de espera, y su semaforo.
 * @param mateSem: mateSem destruido
 */
void mateSem_destroy(t_mateSem* mateSem);

/**
 * @DESC: Pone en espera a un proceso (carpincho) en el semaforo mateSem. 
 *        Pasa de largo y resta 1 si el contador es > 0.
 * @param mateSem: el mateSem usado
 * @param process: proceso (carpincho) que espera
 */
processState mateSem_wait(t_mateSem* mateSem, t_process* process, t_processQueues direcciones);

/**
 * @DESC: Incrementa el contador del mateSem en 1
 * @param mateSem: mateSem en cuestion
 */
void mateSem_post(t_mateSem* mateSem, t_processQueues direcciones);

typedef enum processState {CONTINUE, BLOCK, EXIT} processState; 

typedef struct t_processQueues {
    t_pQueue *newQueue;
    t_pQueue *readyQueue;
    t_pQueue *blockedQueue;
    t_pQueue *suspendedReadyQueue;
    t_pQueue *suspendedBlockedQueue;
    t_pQueue *execQueue;
} t_processQueues;


#endif // !MATESEM_H_