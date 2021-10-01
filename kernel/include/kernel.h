#ifndef  KERNEL_H_
#define  KERNEL_H_

#include "pQueue.h"
#include "process.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "kernelConfig.h"

#define MAX_MULTIPROCESSING 10

//Colas de estado compartidas
t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedReadyQueue, *suspendedBlockedQueue;

pthread_t thread_longTerm, thread_mediumTerm;
//Ver tema implementación shortTerm planner

t_kernelConfig* config;

void *auxHandler(void *vclientSocket);

void *longTerm_run(void* args);

#endif // !KERNEL_H_
