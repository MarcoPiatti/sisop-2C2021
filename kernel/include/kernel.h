#ifndef  KERNEL_H_
#define  KERNEL_H_

#include "pQueue.h"
#include "process.h"
#include "IODevice.h"
#include "kernelConfig.h"
#include "networking.h"

#include <commons/collections/dictionary.h>

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

t_kernelConfig *kernelConfig;

t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedReadyQueue;

sem_t sem_multiprogram;

pthread_t thread_longTerm, thread_mediumTerm;

pthread_t *thread_CPUs;

t_dictionary *IO_dict, *sem_dict;

void* thread_longTermFunc(void* args);

void* thread_mediumTermFunc(void* args);

void* thread_CPUFunc(void* args);

void* thread_IODeviceFunc(void* args);

void* thread_semFunc(void* args);

extern const t_packet*(*petitionHandlers[MAX_PETITIONS])(t_packet* petition);

#endif // !KERNEL_H_
