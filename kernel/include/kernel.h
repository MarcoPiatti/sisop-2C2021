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
#include "commons/collections/dictionary.h"
#include "deadlockDetector.h"
#include <time.h>
#include <string.h>
#include <IODevice.h>

//Aux
t_log *kernelLogger;
pthread_mutex_t mutex_log, mutex_sem_dict, mutex_mediumTerm, mutex_IO_dict;
pthread_cond_t cond_mediumTerm;
u_int32_t memSocket;

t_dictionary *sem_dict;

sem_t sem_multiprogram, availableCPUS, runShortTerm;

#define MAX_MULTIPROCESSING 10

//Colas de estado compartidas
t_pQueue *newQueue, *readyQueue, *blockedQueue, *suspendedReadyQueue, *suspendedBlockedQueue, *execQueue;


t_dictionary* mateSems, *IO_dict;

t_deadlockDetector* dd;

pthread_t thread_longTerm, thread_mediumTerm, *thread_Cpus;
//Ver tema implementación shortTerm planner

t_kernelConfig* config;

t_deadlockDetector* deadlockDetector;

void *auxHandler(void *vclientSocket);

void *longTerm_run(void* args);

void *shortTerm_run(void* args);

void updateWaited(t_process* process);

void *cpu(void* args);

void* thread_IODeviceFunc(void* args);

void* thread_semFunc(void* args);

bool compareSJF(t_process* p1, t_process* p2);

int responseRatio(t_process* process);

bool compareHRRN(t_process* p1, t_process* p2);

processState (*petitionProcessHandler[MAX_PETITIONS])(t_packet *received, t_process* process, int memSocket);

void *deadlockDetector_thread(void* args);

bool findDeadlocks(t_deadlockDetector* dd, int memorySocket);

#endif // !KERNEL_H_
