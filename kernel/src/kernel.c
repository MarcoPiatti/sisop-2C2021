#include "kernel.h"
#include <commons/temporal.h>
#include <string.h>
#include <signal.h>

void sigintHandler(int unused){

    void _mateSem_destroy(void *sem){
        mateSem_destroy((t_mateSem*) sem);
    }
    dictionary_clean_and_destroy_elements(sem_dict, _mateSem_destroy);
    exit(EXIT_SUCCESS);
}

double rr(t_process* process){
    return 1 + (process->waitedTime / process->estimate);
}

// Funcion para poner un proceso a ready, actualiza la cola de ready y la reordena segun algoritmo
// No hay hilo de corto plazo ya que esta funcion hace exactamente eso de un saque
void putToReady(t_process* process){
    pQueue_lock(readyQueue);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
        double timeSinceLastReschedule = (double)(stop.tv_sec - start.tv_sec) * 1000 
                                       + (double)(stop.tv_nsec - start.tv_nsec) / 1000000;
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

        void _printEstimators(void* elem){
            t_process* proc = (t_process*)elem;
            log_info(logger, "Proceso %i - estimador: %f, rr: %f", proc->pid, proc->estimate, rr(proc));
        };

        pthread_mutex_lock(&mutex_log);
        printf("\n");
        log_info(logger, "Corto Plazo: Cola Ready replanificada:");
        pQueue_iterate(readyQueue, _printEstimators);
        printf("\n");
        pthread_mutex_unlock(&mutex_log);

    pQueue_unlock(readyQueue);
}

bool SJF(void*elem1, void*elem2){
    return ((t_process*)elem1)->estimate <= ((t_process*)elem2)->estimate;
}

bool HRRN(void*elem1, void*elem2){
    t_process* process1 = (t_process*)elem1;
    t_process* process2 = (t_process*)elem2;
    double RR1 = rr(process1);
    double RR2 = rr(process2);
    return RR1 >= RR2;
}

// Hilo del largo plazo, toma un proceso de new o suspended_ready y lo pasa a ready
void* thread_longTermFunc(void* args){
    t_process *process;
    while(1){
        sem_wait(&longTermSem);
        pthread_mutex_lock(&mutex_mediumTerm);
            process = (t_process*)pQueue_take(newQueue);
            process->state = READY;

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Largo Plazo: el proceso %u pasa de NEW a READY", process->pid);
            pthread_mutex_unlock(&mutex_log);

            putToReady(process);

            sem_wait(&sem_multiprogram);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);
    }
}

void* thread_mediumTermUnsuspenderFunc(void* args){
    t_process *process;
    while(1){
        sem_wait(&sem_multiprogram);
        sem_post(&sem_multiprogram);
        sem_wait(&sem_newProcess);
        pthread_mutex_lock(&mutex_mediumTerm);
            if(pQueue_isEmpty(suspendedReadyQueue)){
                sem_post(&longTermSem);
                pthread_mutex_unlock(&mutex_mediumTerm);
            }
            process = (t_process*)pQueue_take(suspendedReadyQueue);
            process->state = READY;

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Mediano Plazo: el proceso %u pasa de SUSPENDED-READY a READY", process->pid);
            pthread_mutex_unlock(&mutex_log);

            putToReady(process);

            sem_wait(&sem_multiprogram);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);
    }
}

// Hilo de mediano plazo, se despierta solo cuando el grado de multiprogramacion esta copado de procesos en blocked
// Agarra un proceso de blocked, lo pasa a suspended blocked y sube el grado de multiprogramacion
void* thread_mediumTermFunc(void* args){
    t_process *process;
    int availablePrograms;

    int memorySocket = connectToServer(kernelConfig->memoryIP, kernelConfig->memoryPort);

    t_packet* suspendRequest;

    while(1){
        pthread_mutex_lock(&mutex_mediumTerm);
            //Espera a que se cumpla la condicion para despertarse
            sem_getvalue(&sem_multiprogram, &availablePrograms);
            while(availablePrograms >= 1 || pQueue_isEmpty(newQueue) || !pQueue_isEmpty(readyQueue) || pQueue_isEmpty(blockedQueue)){
                pthread_cond_wait(&cond_mediumTerm, &mutex_mediumTerm);
                sem_getvalue(&sem_multiprogram, &availablePrograms);
            }
            //Sacamos al proceso de la cola de blocked y lo metemos a suspended blocked
            process = (t_process*)pQueue_takeLast(blockedQueue);
            process->state = SUSP_BLOCKED;
            pQueue_put(suspendedBlockedQueue, (void*)process);
            sem_post(&sem_multiprogram);

            //Notifica a memoria de la suspension
            suspendRequest = createPacket(SUSPEND, INITIAL_STREAM_SIZE);
            streamAdd_UINT32(suspendRequest->payload, process->pid);
            socket_sendPacket(memorySocket, suspendRequest);
            destroyPacket(suspendRequest);
        pthread_mutex_unlock(&mutex_mediumTerm);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Mediano Plazo: el proceso %u pasa a suspended blocked", process->pid);
        pthread_mutex_unlock(&mutex_log);
    }
}

// Hilo CPU, toma un proceso de ready y ejecuta todas sus peticiones hasta que se termine o pase a blocked
// Cuando lo pasa a blocked, recalcula el estimador de rafaga del proceso segun la cantidad de rafagas que duro
void* thread_CPUFunc(void* args){
    intptr_t CPUid = (intptr_t)args;
    t_process *process = NULL;
    t_packet *request = NULL;
    bool keepServing = true;
    struct timespec rafagaStart, rafagaStop;

    int memorySocket = connectToServer(kernelConfig->memoryIP, kernelConfig->memoryPort);

    while(1){
        process = NULL;
        keepServing = true;
        process = pQueue_take(readyQueue);
        process->state = EXEC;
        
        pthread_mutex_lock(&mutex_mediumTerm);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);

        t_packet* ok_packet = createPacket(OK, 0);
        socket_sendPacket(process->socket, ok_packet);
        destroyPacket(ok_packet);

        clock_gettime(CLOCK_MONOTONIC, &rafagaStart);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "CPU %i: el proceso %u pasa de READY a EXEC", CPUid, process->pid);
        pthread_mutex_unlock(&mutex_log);

        while(keepServing){
            request = socket_getPacket(process->socket);
            if(request == NULL){
                if(!retry_getPacket(process->socket, &request)){
                    t_packet* abruptTerm = createPacket(CAPI_TERM, INITIAL_STREAM_SIZE);
                    streamAdd_UINT32(abruptTerm->payload, process->pid);
                    petitionHandlers[CAPI_TERM](process, abruptTerm, memorySocket);
                    destroyPacket(abruptTerm);
                    petitionHandlers[DISCONNECTED](process, request, memorySocket);
                    break;
                }
            }
            keepServing = petitionHandlers[request->header](process, request, memorySocket);

            if(request->header == SEM_WAIT || request->header == CALL_IO){
                clock_gettime(CLOCK_MONOTONIC, &rafagaStop);
                double rafagaMs = (double)(rafagaStop.tv_sec - rafagaStart.tv_sec) * 1000 
                                + (double)(rafagaStop.tv_nsec - rafagaStart.tv_nsec) / 1000000;
                double oldEstimate = process->estimate;
                process->estimate = kernelConfig->alpha * rafagaMs + (1 - kernelConfig->alpha) * process->estimate;
                pthread_mutex_lock(&mutex_log);
                log_info(logger, "Proceso %u: Nueva estimacion - rafaga real finalizada: %f, estimador viejo: %f, estimador nuevo: %f", process->pid, rafagaMs, oldEstimate, process->estimate);
                pthread_mutex_unlock(&mutex_log);
            }

            destroyPacket(request);
        }
    }
}

// Hilo Dispositivo IO, toma un proceso de su cola de espera, lo "procesa" y lo manda a la cola respectiva
// Si estaba en blocked, lo manda a ready, si estaba en suspended blocked, lo manda a suspended Ready
void* thread_IODeviceFunc(void* args){
    t_IODevice* self = (t_IODevice*) args;
    t_process* process = NULL;
    bool matchesPid(void* elem){
        return process->pid == ((t_process*)elem)->pid;
    };

    while(1){
        process = (t_process*)pQueue_take(self->waitingProcesses);
        
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Dispositivo IO %s: el proceso %u se bloquea %ims", self->nombre, process->pid, self->duracion);
        pthread_mutex_unlock(&mutex_log);
        
        for(int i = 0; i < self->duracion; i++){
            usleep(1000);
        }
        
        if(process->state == BLOCKED){
            pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_removeBy(blockedQueue, matchesPid);
            pthread_mutex_unlock(&mutex_mediumTerm);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Dispositivo IO %s: el proceso %u pasa de BLOCKED a READY", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);

            putToReady(process);
        }
        else if(process->state == SUSP_BLOCKED){
            pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pthread_mutex_unlock(&mutex_mediumTerm);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Dispositivo IO %s: el proceso %u pasa de SUSPENDED-BLOCKED a SUSPENDED-READY", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);

            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);
        }
    }
}

// Hilo para semaforo, se despierta solo si el contador < 1 y hay algun proceso en espera
// Toma un proceso y lo pone en ready o suspended ready respectivamente
void* thread_semFunc(void* args){
    t_mateSem* self = (t_mateSem*) args;
    t_process* process = NULL;
    bool matchesPid(void* elem){
        return process->pid == ((t_process*)elem)->pid;
    };

    while(1){
        pthread_mutex_lock(&self->sem_mutex);
        while(pQueue_isEmpty(self->waitingProcesses) || !self->sem){
            pthread_cond_wait(&self->sem_cond, &self->sem_mutex);
        }
        process = (t_process*)pQueue_take(self->waitingProcesses);
        self->sem--;
        pthread_mutex_unlock(&self->sem_mutex);
        if(process->state == BLOCKED){
            pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_removeBy(blockedQueue, matchesPid);
            pthread_mutex_unlock(&mutex_mediumTerm);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Semaforo %s: el proceso %u pasa de BLOCKED a READY", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
            putToReady(process);
        }
        else if(process->state == SUSP_BLOCKED){
            pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pthread_mutex_unlock(&mutex_mediumTerm);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Semaforo %s: el proceso %u pasa de SUSPENDED-BLOCKED a SUSPENDED-READY", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);
        }

        DDSemAllocated(dd, process, self);
    }
}

// Hilo deadlock detector. Se despierta cada tanto y revisa si hay deadlocks 
void* thread_deadlockDetectorFunc(void* args){
    t_deadlockDetector* self = (t_deadlockDetector*)args;

    int memorySocket = connectToServer(kernelConfig->memoryIP, kernelConfig->memoryPort);

    while(1){
        for(int i = 0; i < kernelConfig->DeadlockDelay; i++){
            usleep(1000);
        }
        pthread_mutex_lock(&dd->mutex);
        while(findDeadlocks(self, memorySocket));
        pthread_mutex_unlock(&dd->mutex);
    }
}

// Hilo main del kernel, inicializa todo, crea a los otros hilo y hace de server para recibir nuevos procesos y mandarlos a ready
int main(){

    signal(SIGINT, sigintHandler);

    logger = log_create("./cfg/kernel.log", "Kernel", true, LOG_LEVEL_TRACE);
    pthread_mutex_init(&mutex_log, NULL);

    kernelConfig = getKernelConfig("./cfg/kernel.config");

    // Inicializar estructuras de estado 
    newQueue = pQueue_create();
    readyQueue = pQueue_create();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); //Reloj de espera de ready
    blockedQueue = pQueue_create();
    suspendedBlockedQueue = pQueue_create();
    suspendedReadyQueue = pQueue_create();

    // Se toma el algoritmo de planificacion
    if(!strcmp(kernelConfig->schedulerAlgorithm, "SJF"))
        sortingAlgoritm = SJF;
    if(!strcmp(kernelConfig->schedulerAlgorithm, "HRRN"))
        sortingAlgoritm = HRRN;

    // Inicializar semaforo de multiprocesamiento
    sem_init(&sem_multiprogram, 0, kernelConfig->multiprogram);
    sem_init(&sem_newProcess, 0, 0);
    sem_init(&longTermSem, 0, 0);

    // Inicializo condition variable para despertar al planificador de mediano plazo 
    pthread_cond_init(&cond_mediumTerm, NULL);
    pthread_mutex_init(&mutex_mediumTerm, NULL);

    /* Inicializar CPUs */
    thread_CPUs = malloc(sizeof(pthread_t) * kernelConfig->multiprocess);
    for(intptr_t i = 0; i < kernelConfig->multiprocess; i++){
        pthread_create(thread_CPUs + i, 0, thread_CPUFunc, (void*)i);
        pthread_detach(thread_CPUs[i]);
    }

    // Inicializar Dispositivos IO 
    IO_dict = dictionary_create();
    for(int i = 0; kernelConfig->IODeviceNames[i]; i++){
        dictionary_put( IO_dict,
                        kernelConfig->IODeviceNames[i],
                        createIODevice( kernelConfig->IODeviceNames[i],
                                        atoi(kernelConfig->IODeviceDelays[i]),
                                        thread_IODeviceFunc));
    }
    pthread_mutex_init(&mutex_IO_dict, NULL);

    // Crear estructura para agregar semaforos
    sem_dict = dictionary_create();
    pthread_mutex_init(&mutex_sem_dict, NULL);


    // Inicializar Planificador de largo plazo
    pthread_create(&thread_longTerm, 0, thread_longTermFunc, NULL);
    pthread_detach(thread_longTerm);

    pthread_create(&thread_mediumTermUnsuspender, 0, thread_mediumTermUnsuspenderFunc, NULL);
    pthread_detach(thread_mediumTermUnsuspender);
    
    // Inicializar Planificador de mediano plazo
    pthread_create(&thread_mediumTerm, 0, thread_mediumTermFunc, NULL);
    pthread_detach(thread_mediumTerm);

    // Inicializar detector de deadlocks
    dd = createDeadlockDetector(thread_deadlockDetectorFunc);
    // Inicializar servidor
    int serverSocket = createListenServer(kernelConfig->kernelIP, kernelConfig->kernelPort);
    
    // El main hace de server, escucha conexiones nuevas y las pone en new
    t_process* process = NULL;
    int memSock = connectToServer(kernelConfig->memoryIP, kernelConfig->memoryPort);
    while(1){
        int newProcessSocket = getNewClient(serverSocket);

        t_packet* idPacket = socket_getPacket(newProcessSocket);
        if(idPacket == NULL){
            if(!retry_getPacket(newProcessSocket, &idPacket)){
                close(newProcessSocket);
                continue;
            }
        }

        uint32_t pid = streamTake_UINT32(idPacket->payload);
        socket_relayPacket(memSock, idPacket);
        socket_ignoreHeader(memSock);
        t_packet* ignored = socket_getPacket(memSock);
        destroyPacket(ignored);
        socket_sendHeader(newProcessSocket, ID_KERNEL);

        process = createProcess(pid, newProcessSocket, kernelConfig->initialEstimator);

        pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_put(newQueue, process);
            sem_post(&sem_newProcess);
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Proceso %u: se conecta", pid);
            pthread_mutex_unlock(&mutex_log);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);
        
        DDProcInit(dd, process);

        destroyPacket(idPacket);
    }

    return 0;
}