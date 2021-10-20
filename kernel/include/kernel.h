/**
 * @file: kernel.h
 * @author pepinOS 
 * @DESC: Main del modulo Kernel del TP Carpinchos 2C2021
 * @version 0.1
 * @date: 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef  KERNEL_H_
#define  KERNEL_H_

#include "pQueue.h"
#include "mateSem.h"
#include "process.h"
#include "IODevice.h"
#include "deadlockDetector.h"
#include "kernelConfig.h"
#include "networking.h"
#include "headers.h"

#include <commons/collections/dictionary.h>

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Datos de la config
t_kernelConfig *kernelConfig;

// Colas de estado de planificacion
t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedBlockedQueue, *suspendedReadyQueue;

// Instancia de un Deadlock detector
t_deadlockDetector* dd;

// Semaforos para el planificador de largo plazo
// El primero refleja el grado de multiprogramacion
// El segundo indica si hay nuevos procesos
sem_t sem_multiprogram, sem_newProcess;

// Hilos para el planificador de largo y mediano plazo
pthread_t thread_longTerm, thread_mediumTerm;

// Condition variable para despertar al planificador de medio plazo
pthread_cond_t cond_mediumTerm;
pthread_mutex_t mutex_mediumTerm;

// Array con los hilos CPU
pthread_t *thread_CPUs;

//Diccionarios y sus mutexes
t_dictionary *IO_dict, *sem_dict;
pthread_mutex_t mutex_IO_dict, mutex_sem_dict;

// Timers usados para medir el tiempo de la cola de ready para HRRN
struct timespec start, stop;

// Puntero a funcion a modo de container para elegir SJF o HRRN
bool(*sortingAlgoritm)(void*, void*);

bool SJF(void*, void*); //compara segun estimador
bool HRRN(void*, void*); //compara segun RR: tiempoEsperado / estimador

void* thread_longTermFunc(void* args); 

void* thread_mediumTermFunc(void* args); 

void* thread_CPUFunc(void* args);

void* thread_IODeviceFunc(void* args);

void* thread_semFunc(void* args);

void* thread_deadlockDetectorFunc(void* args);

//Array con funciones para procesar cada posible pedido de los procesos
extern bool(*petitionHandlers[MAX_PETITIONS])(t_process* process, t_packet* petition, int memorySocket);

//Funcion para hallar deadlocks y terminar un proceso
bool findDeadlocks(t_deadlockDetector* dd, int memorySocket);

#endif // !KERNEL_H_
