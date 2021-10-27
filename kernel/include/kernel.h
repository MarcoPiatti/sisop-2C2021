#ifndef  KERNEL_H_
#define  KERNEL_H_

#include "networking.h"
#include "pQueue.h"
#include "process.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "kernelConfig.h"
#include "mateSem.h"

#define MAX_MULTIPROCESSING 10

//Colas de estado compartidas
t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedReadyQueue, *suspendedBlockedQueue, *execQueue;

pthread_t thread_longTerm, thread_mediumTerm, *thread_Cpus;
//Ver tema implementación shortTerm planner

t_kernelConfig* config;

void *auxHandler(void *vclientSocket);

void *longTerm_run(void* args);

void *shortTerm_run(void* args);

void updateWaited(t_process* process);

void *cpu(void* args);

bool compareSJF(t_process* p1, t_process* p2);

int responseRatio(t_process* process);

bool compareHRRN(t_process* p1, t_process* p2);

typedef enum processState {CONTINUE, BLOCK, EXIT} processState; 

processState (*petitionProcessHandler[MAX_PETITIONS])(t_packet *received, int clientSocket);

#endif // !KERNEL_H_
