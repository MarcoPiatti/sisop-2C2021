/**
 * @file: mateSem.h
 * @author pepinOS 
 * @DESC: TAD para semaforos "mate", para su uso en el kernel.
 * @version 0.1
 * @date: 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef MATESEM_H_
#define MATESEM_H_

#include <pthread.h>
#include "pQueue.h"
#include "process.h"

/**
 * @DESC: Semaforo para carpinchos.
 * - nombre: nombre.
 * - sem: contador del semaforo.
 * - sem_cond: condition variable para despertar al semaforo cuando tenga que pasar alguien
 * - sem_mutex: mutex de la contidion variable
 * - waitingProcesses: cola de procesos que esperan a usar al semaforo
 * - thread_mateSem: hilo que hara de semaforo, dejando pasar procesos
 */
typedef struct mateSem {
    char* nombre;
    unsigned int sem;
    pthread_cond_t sem_cond;
    pthread_mutex_t sem_mutex;
    t_pQueue* waitingProcesses;
    pthread_t thread_mateSem;
} t_mateSem;

/**
 * @DESC: Crea un mateSem en memoria
 * @param nombre: nombre del mateSem
 * @param contadorInicial: valor inicial del contador.
 * @param mateSemFunc: funcion que ejecutara el thread del mateSem.
 * @return t_mateSem*: puntero al struct del mateSem creado
 */
t_mateSem* mateSem_create(char* nombre, unsigned int contadorInicial, void* (* mateSemFunc)(void*));

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
bool mateSem_wait(t_mateSem* mateSem, t_process* process);

/**
 * @DESC: Incrementa el contador del mateSem en 1
 * @param mateSem: mateSem en cuestion
 */
void mateSem_post(t_mateSem* mateSem);

#endif // !MATESEM_H_