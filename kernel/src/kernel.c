#include "kernel.h"
#include <commons/temporal.h>

void putToReady(t_process* process){
    pQueue_lock(readyQueue);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
    time_t timeSinceLastReschedule = (stop.tv_sec - start.tv_sec);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    void updateMetrics(void* elem){
        t_process* process = (t_process*)elem;
        process->waitedTime += timeSinceLastReschedule;
    };
    pQueue_iterate(readyQueue, updateMetrics);

    process->waitedTime = 0;
    pQueue_put(readyQueue, (void*)process);
    pQueue_sort(readyQueue, sortingAlgoritm);
    pQueue_unlock(readyQueue);
}

bool SJF(void*elem1, void*elem2){
    return ((t_process*)elem1)->estimate < ((t_process*)elem2)->estimate;
}

bool HRRN(void*elem1, void*elem2){
    t_process* process1 = (t_process*)elem1;
    t_process* process2 = (t_process*)elem2;
    double RR1 = (double)process1->waitedTime / process1->estimate;
    double RR2 = (double)process2->waitedTime / process2->estimate;
    return RR1 > RR2;
}


void* thread_longTermFunc(void* args){
    t_process *process;
    while(1){
        sem_post(&sem_mediumLong);
        sem_wait(&sem_multiprogram);
        sem_wait(&sem_newProcess);
        if(pQueue_isEmpty(suspendedReadyQueue))
            process = (t_process*)pQueue_take(newQueue);
        else process = (t_process*)pQUeue_take(suspendedReadyQueue);
        process->state = READY;
        putToReady(process);
    }
}

void* thread_mediumTermFunc(void* args){
    t_process *process;
    int availablePrograms;
    while(1){
        sem_getvalue(&sem_multiprogram, &availablePrograms);
        while(availablePrograms >= 1 || pQueue_isEmpty(newQueue) || !pQueue_isEmpty(readyQueue) || pQueue_isEmpty(blockedQueue)){
            sem_wait(&sem_mediumLong);
            sem_getvalue(&sem_multiprogram, &availablePrograms);     
        }
        process = (t_process*)pQueue_peekLast(blockedQueue);
        process->state = SUSP_BLOCKED;
        pQueue_put(&suspendedBlockedQueue, (void*)process);
        sem_post(&sem_multiprogram);
    }
}

void* thread_CPUFunc(void* args){
    t_process *process;
    t_packet *request;
    int memorySocket = connectToServer(kernelConfig->memoryIP, kernelConfig->memoryPort);
    int rafaga;
    bool keepServing = true;
    while(1){
        rafaga = 0;
        process = pQueue_take(readyQueue);
        while(keepServing){
            request = socket_getPacket(process->socket);
            for(int i = 0; i < kernelConfig->CPUDelay; i++){
                usleep(1000);
            }
            rafaga++;
            if(request->header == SEM_WAIT || request->header == CALL_IO)
                process->estimate = kernelConfig->alpha * (double)rafaga
                                    + (1 - kernelConfig->alpha) * process->estimate;
            keepServing = petitionHandlers[request->header](process, request, memorySocket);
            destroyPacket(request);
        }
    }
}

void* thread_IODeviceFunc(void* args){
    t_IODevice* self = (t_IODevice*) args;
    t_process* process;
    t_packet *response;
    bool matchesPid(void* elem){
        return process->pid == ((t_process*)elem)->pid;
    };

    while(1){
        process = (t_process*)pQueue_take(self->waitingProcesses);
        for(int i = 0; i < self->duracion; i++){
            usleep(1000);
        }
        if(process->state == BLOCKED){
            pQueue_removeBy(blockedQueue, matchesPid);
            putToRead(process);
        }
        else if(process->state == SUSP_BLOCKED){
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);
        }
        response = createPacket(OK, 0);
        socket_sendPacket(process->socket, response);
        free(response);
    }
}

void* thread_semFunc(void* args){
    t_mateSem* self = (t_mateSem*) args;
    t_process* process;
    t_packet *response;
    bool matchesPid(void* elem){
        return process->pid == ((t_process*)elem)->pid;
    };

    while(1){
        sem_wait(&self->sem);
        process = (t_process*)pQueue_take(self->waitingProcesses);
        if(process->state == BLOCKED){
            pQueue_removeBy(blockedQueue, matchesPid);
            putToRead(process);
        }
        else if(process->state == SUSP_BLOCKED){
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);
        }
        response = createPacket(OK, 0);
        socket_sendPacket(process->socket, response);
        free(response);
    }
}

/* El main hace de server, escucha conexiones nuevas y las pone en new */
int main(){
    kernelConfig = getKernelConfig("./cfg/kernel.config");

    /* Inicializar estructuras de estado */
    newQueue = pQueue_create();
    readyQueue = pQueue_create();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); //Reloj de espera de ready
    blockedQueue = pQueue_create();
    suspendedBlockedQueue = pQueue_create();
    suspendedReadyQueue = pQueue_create();

    if(!strcmp(kernelConfig->schedulerAlgorithm, "SJF"))
        sortingAlgoritm = SJF;
    if(!strcmp(kernelConfig->schedulerAlgorithm, "HRRN"))
        sortingAlgoritm = HRRN;

    /* Inicializar semaforo de multiprocesamiento */
    sem_init(&sem_multiprogram, 0, kernelConfig->multiprogram);
    sem_init(&sem_newProcess, 0, 0);
    sem_init(&sem_mediumLong, 0, 0);

    /* Inicializar CPUs */
    thread_CPUs = malloc(sizeof(pthread_t) * kernelConfig->multiprocess);
    for(int i = 0; i < kernelConfig->multiprocess; i++){
        pthread_create(thread_CPUs + i, 0, thread_CPUFunc, NULL);
        pthread_detach(thread_CPUs[i]);
    }

    /* Inicializar Dispositivos IO */
    IO_dict = dictionary_create();
    for(int i = 0; kernelConfig->IODeviceNames[i]; i++){
        dictionary_put( IO_dict,
                        kernelConfig->IODeviceNames[i],
                        createIODevice( kernelConfig->IODeviceNames[i],
                                        kernelConfig->IODeviceDelays[i],
                                        thread_IODeviceFunc));
    }

    /* Crear estructura para agregar semaforos */
    sem_dict = dictionary_create();

    /* Inicializar Planificador de largo plazo */
    pthread_create(&thread_longTerm, 0, thread_longTermFunc, NULL);
    pthread_detach(thread_longTerm);
    
    /* Inicializar Planificador de mediano plazo */
    pthread_create(&thread_mediumTerm, 0, thread_mediumTermFunc, NULL);
    pthread_detach(thread_mediumTerm);

    /* Inicializar servidor */
    int serverSocket = createListenServer(kernelConfig->kernelIP, kernelConfig->kernelPort);
    
    while(1){
        int newProcessSocket = getNewClient(serverSocket);
        socket_sendHeader(newProcessSocket, ID_KERNEL);
        pQueue_put(newQueue, createProcess(newProcessSocket, newProcessSocket, kernelConfig->initialEstimator));
        sem_post(&sem_newProcess);
    }

    return 0;
}