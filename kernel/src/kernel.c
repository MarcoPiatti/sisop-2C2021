#include "kernel.h"
#include <commons/temporal.h>
#include <string.h>

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
        process->state = READY;
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
        sem_wait(&sem_multiprogram);
        sem_post(&sem_multiprogram);
        sem_wait(&sem_newProcess);
        pthread_mutex_lock(&mutex_mediumTerm);
            if(pQueue_isEmpty(suspendedReadyQueue))
                process = (t_process*)pQueue_take(newQueue);
            else process = (t_process*)pQueue_take(suspendedReadyQueue);
            process->state = READY;
            putToReady(process);
            sem_wait(&sem_multiprogram);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Largo Plazo: el proceso %i pasa a ready", process->pid);
        pthread_mutex_unlock(&mutex_log);
    }
}

void* thread_mediumTermFunc(void* args){
    t_process *process;
    int availablePrograms;
    while(1){
        pthread_mutex_lock(&mutex_mediumTerm);
            sem_getvalue(&sem_multiprogram, &availablePrograms);
            while(availablePrograms >= 1 || pQueue_isEmpty(newQueue) || !pQueue_isEmpty(readyQueue) || pQueue_isEmpty(blockedQueue)){
                pthread_cond_wait(&cond_mediumTerm, &mutex_mediumTerm);
                sem_getvalue(&sem_multiprogram, &availablePrograms);
            }
            process = (t_process*)pQueue_takeLast(blockedQueue);
            process->state = SUSP_BLOCKED;
            pQueue_put(suspendedBlockedQueue, (void*)process);
            sem_post(&sem_multiprogram);
        pthread_mutex_unlock(&mutex_mediumTerm);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Mediano Plazo: el proceso %i pasa a suspended blocked", process->pid);
        pthread_mutex_unlock(&mutex_log);
    }
}

void* thread_CPUFunc(void* args){
    intptr_t CPUid = (intptr_t)args;
    t_process *process = NULL;
    t_packet *request = NULL;
    int memorySocket = connectToServer(kernelConfig->memoryIP, kernelConfig->memoryPort);
    int rafaga = 0;
    bool keepServing = true;
    while(1){
        process = NULL;
        rafaga = 0;
        keepServing = true;
        process = pQueue_take(readyQueue);
        process->state = EXEC;
        pthread_mutex_lock(&mutex_mediumTerm);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "CPU %i: el proceso %i pasa a exec", CPUid, process->pid);
        pthread_mutex_unlock(&mutex_log);

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
    t_process* process = NULL;
    t_packet *response = NULL;
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
            putToReady(process);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Dispositivo IO %s: el proceso %i pasa a ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
        }
        else if(process->state == SUSP_BLOCKED){
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Dispositivo IO %s: el proceso %i pasa a suspended ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
        }
        response = createPacket(OK, 0);
        socket_sendPacket(process->socket, response);
        free(response);
    }
}

/*TODO: Ver como limpar bien el thread cuando se lo cancela para evitar leaks
      Referencias: pthread_cleanup_push
*/
void* thread_semFunc(void* args){
    t_mateSem* self = (t_mateSem*) args;
    t_process* process = NULL;
    t_packet *response = NULL;
    bool matchesPid(void* elem){
        return process->pid == ((t_process*)elem)->pid;
    };

    while(1){
        sem_wait(&self->sem);
        process = (t_process*)pQueue_take(self->waitingProcesses);
        if(process->state == BLOCKED){
            pQueue_removeBy(blockedQueue, matchesPid);
            putToReady(process);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Semaforo %s: el proceso %i pasa a ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
        }
        else if(process->state == SUSP_BLOCKED){
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Semaforo %s: el proceso %i pasa a suspended ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
        }
        response = createPacket(OK, 0);
        socket_sendPacket(process->socket, response);
        destroyPacket(response);
    }
}

int main(){
    logger = log_create("./cfg/kernel.log", "kernel", true, LOG_LEVEL_TRACE);
    pthread_mutex_init(&mutex_log, NULL);

    kernelConfig = getKernelConfig("./cfg/kernel.config");

    /* Inicializar estructuras de estado */
    newQueue = pQueue_create();
    readyQueue = pQueue_create();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); //Reloj de espera de ready
    blockedQueue = pQueue_create();
    suspendedBlockedQueue = pQueue_create();
    suspendedReadyQueue = pQueue_create();

    /* Se toma el algoritmo de planificacion */
    if(!strcmp(kernelConfig->schedulerAlgorithm, "SJF"))
        sortingAlgoritm = SJF;
    if(!strcmp(kernelConfig->schedulerAlgorithm, "HRRN"))
        sortingAlgoritm = HRRN;

    /* Inicializar semaforo de multiprocesamiento */
    sem_init(&sem_multiprogram, 0, kernelConfig->multiprogram);
    sem_init(&sem_newProcess, 0, 0);

    /* Inicializo condition variable para despertar al planificador de mediano plazo */
    pthread_cond_init(&cond_mediumTerm, NULL);
    pthread_mutex_init(&mutex_mediumTerm, NULL);

    /* Inicializar CPUs */
    thread_CPUs = malloc(sizeof(pthread_t) * kernelConfig->multiprocess);
    for(intptr_t i = 0; i < kernelConfig->multiprocess; i++){
        pthread_create(thread_CPUs + i, 0, thread_CPUFunc, (void*)i);
        pthread_detach(thread_CPUs[i]);
    }

    /* Inicializar Dispositivos IO */
    IO_dict = dictionary_create();
    for(int i = 0; kernelConfig->IODeviceNames[i]; i++){
        dictionary_put( IO_dict,
                        kernelConfig->IODeviceNames[i],
                        createIODevice( kernelConfig->IODeviceNames[i],
                                        atoi(kernelConfig->IODeviceDelays[i]),
                                        thread_IODeviceFunc));
    }
    pthread_mutex_init(&mutex_IO_dict, NULL);

    /* Crear estructura para agregar semaforos */
    sem_dict = dictionary_create();
    pthread_mutex_init(&mutex_sem_dict, NULL);


    /* Inicializar Planificador de largo plazo */
    pthread_create(&thread_longTerm, 0, thread_longTermFunc, NULL);
    pthread_detach(thread_longTerm);
    
    /* Inicializar Planificador de mediano plazo */
    pthread_create(&thread_mediumTerm, 0, thread_mediumTermFunc, NULL);
    pthread_detach(thread_mediumTerm);

    /* Inicializar servidor */
    int serverSocket = createListenServer(kernelConfig->kernelIP, kernelConfig->kernelPort);
    
    /* El main hace de server, escucha conexiones nuevas y las pone en new */
    while(1){
        int newProcessSocket = getNewClient(serverSocket);
        socket_sendHeader(newProcessSocket, ID_KERNEL);
        pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_put(newQueue, createProcess(newProcessSocket, newProcessSocket, kernelConfig->initialEstimator));
            sem_post(&sem_newProcess);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: se conecta", newProcessSocket);
        pthread_mutex_unlock(&mutex_log);
    }

    return 0;
}