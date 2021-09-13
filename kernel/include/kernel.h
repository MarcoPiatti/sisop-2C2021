#ifndef  KERNEL_H_
#define  KERNEL_H_

#include "pQueue.h"
#include "mateSem.h"
#include "process.h"
#include "IODevice.h"
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

sem_t sem_multiprogram, sem_newProcess, sem_mediumLong;

pthread_t thread_longTerm, thread_mediumTerm;

pthread_t *thread_CPUs;

t_dictionary *IO_dict, *sem_dict;

struct timespec start, stop;

bool(*sortingAlgoritm)(void*, void*);

bool SJF(void*, void*); //compara segun estimador
bool HRRN(void*, void*); //compara segun RR: tiempoEsperado / estimador

void* thread_longTermFunc(void* args); 

void* thread_mediumTermFunc(void* args); 

void* thread_CPUFunc(void* args);

void* thread_IODeviceFunc(void* args);

void* thread_semFunc(void* args);

//Array con funciones para procesar cada posible pedido de los procesos
extern const bool(*petitionHandlers[MAX_PETITIONS])(t_process* process, t_packet* petition, int memorySocket);

#endif // !KERNEL_H_
