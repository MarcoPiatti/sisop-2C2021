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

#include <commons/collections/dictionary.h>

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

t_kernelConfig *kernelConfig;

t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedBlockedQueue, *suspendedReadyQueue;

t_deadlockDetector* dd;

sem_t sem_multiprogram, sem_newProcess;

pthread_t thread_longTerm, thread_mediumTerm;

pthread_cond_t cond_mediumTerm;
pthread_mutex_t mutex_mediumTerm;

pthread_t *thread_CPUs;

t_dictionary *IO_dict, *sem_dict;
pthread_mutex_t mutex_IO_dict, mutex_sem_dict;

struct timespec start, stop;

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
//TODO: Modelar tad de cpu con todos los datos importantes en el struct, y pasarla en lugar de estos argumentos
extern bool(*petitionHandlers[MAX_PETITIONS])(t_process* process, t_packet* petition, int memorySocket);

//Array con funciones para procesar cada posible situacion con el deadlockDetector
extern void(*deadlockHandlers[DD_MAX])(t_deadlockDetector* dd, t_packet* newInfo);

#endif // !KERNEL_H_
