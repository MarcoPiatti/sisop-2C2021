#include "kernel.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include "sleeper.h"

int main(void){
    kernelLogger = log_create("./kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);
    pthread_mutex_init(&mutex_log, NULL);

    config = getKernelConfig("./kernel.cfg");

    //Inicializo estructura t_processQueues
    processQueues.newQueue = newQueue = pQueue_create();
    processQueues.readyQueue = readyQueue = pQueue_create();
    processQueues.blockedQueue = blockedQueue = pQueue_create();
    processQueues.suspendedReadyQueue = suspendedReadyQueue = pQueue_create();
    processQueues.suspendedBlockedQueue = suspendedBlockedQueue = pQueue_create();
    processQueues.execQueue = execQueue = pQueue_create();

    //Inicializo diccionario de mateSems
    mateSems = dictionary_create();

    //Inicializacion semáforos
    sem_init(&cuposDisponibles, 0, config->multiprogram);       //Cantidad de carpinchos que pueden estar en memoria
    sem_init(&availableCPUS, 0, config->multiprocess);          //Cantidad de CPUs disponibles
    sem_init(&runShortTerm, 0, 0);     //Semáforo "notifier" para correr el short term scheduler solo cuando un proceso llega a ready

    //Conecto con memoria. Comentar para testing de solo kernel.
    /* 
    memSocket = connectToServer(config->memoryIP, config->memoryPort);
    if(memSocket) {
        pthread_mutex_lock(&mutex_log);
        log_info(kernelLogger, "Kernel conectado a memoria exitosamente");
        pthread_mutex_unlock(&mutex_log);
    } */
    
    //Inicio hilo sobre el cual corre el long term scheduler
    pthread_create(&thread_longTerm, NULL, longTerm_run, NULL);
    pthread_detach(thread_longTerm);
    thread_Cpus = calloc(config->multiprocess, sizeof(pthread_t));
    for(int i=0;i<config->multiprocess;i++){
        pthread_create(&thread_Cpus[i], NULL, cpu, NULL);
    }

    int serverSocket = createListenServer(config->ip, config->port);

    deadlockDetector = createDeadlockDetector(deadlockDetector_thread);

    while(1){   
        runListenServer(serverSocket, auxHandler);
    }

    close(serverSocket);

    return 0;
}

void *auxHandler(void *vclientSocket){
    int clientSocket = (int) vclientSocket;
    socket_sendHeader(clientSocket, OK);
    
    t_packet *packet;
    t_process* process;
    
    uint32_t idCliente;

    socket_sendHeader(clientSocket, ID_KERNEL);

    packet = socket_getPacket(clientSocket);
    idCliente = streamTake_UINT32(packet->payload);
    process = createProcess(idCliente, clientSocket, config->initialEstimation);
    pQueue_put(newQueue, process);

    pthread_mutex_lock(&mutex_log);
    log_info(kernelLogger, "Proceso %u recibido y agregado a cola de New", process->pid);
    pthread_mutex_unlock(&mutex_log);

    DDProcInit(deadlockDetector, process);

    destroyPacket(packet);

    return NULL; // Ni idea, solo para que no tire warnings, igual esta funcion hay que tirarla completita al tacho.
}

//Toma procesos de la cola de New y los pone en ready cuando sea posible.
void *longTerm_run(void* args) {
    t_process* process;
    while(1) {
        sem_wait(&cuposDisponibles);
        process = pQueue_take(newQueue);
        process->state = READY;
        pQueue_put(readyQueue, process);

        pthread_mutex_lock(&mutex_log);
        log_info(kernelLogger, "El planificador de largo plazo llevo de new a ready al proceso %u", process->pid);
        pthread_mutex_unlock(&mutex_log);

        sem_post(&runShortTerm);
    }
}

void *midTerm_run(void* args) {
    t_process * process;
    while(1) {
        if(!pQueue_isEmpty(newQueue)){
            if(pQueue_isEmpty(readyQueue)) {
                if(!pQueue_isEmpty(blockedQueue)) {
                    // Blocked -> Suspended Blocked
                    process = pQueue_take(blockedQueue);
                    process->state = SUSP_BLOCKED;
                    pQueue_put(suspendedBlockedQueue, process);
                    sem_post(&cuposDisponibles);
                }
            }
        } else {
            if(!pQueue_isEmpty(suspendedReadyQueue)){
                // Suspended Ready -> Ready
                sem_wait(&cuposDisponibles);                        //ver
                process = pQueue_take(suspendedReadyQueue);
                process->state = READY;
                pQueue_put(readyQueue, process);
            }
        }
    }
}


//TODO: Implementar Call io

void *shortTerm_run(void* args) {
    t_process* process;
    void _updateWaited(void* p){
        updateWaited((t_process*) process);
    }
    bool _compareSJF(void *p1, void *p2){
        return compareSJF((t_process*) p1, (t_process*) p2);
    }
    bool _compareHRRN(void *p1, void *p2){
        return compareHRRN((t_process*) p1, (t_process*) p2);
    }
    while(1) {
        sem_wait(&runShortTerm);
        pQueue_iterate(readyQueue, _updateWaited);
        if(! strcmp(config->algorithm,  "SJF"))
            pQueue_sort(readyQueue, _compareSJF);
        else                                        //Se asume que no hay otro algoritmo
            pQueue_sort(readyQueue, _compareHRRN);   
    }
}

// void updateWaited(t_process* process){
//     process->waited += 1; 
// }                                        REDEFINICION DE FUNCION, no se cual es correcta pero deje la otra.

void *cpu(void* args) {

    // TODO: Habria que revisar TODA la funcion, en particular el tema del contador y las comparaciones entre enums.
    int memSock = connectToServer(config->memoryIP, config->memoryPort);
    t_process* process;
    t_packet* packet;
    while(1) {
        
        // TODO: Arreglar contador (siempre queda en 0).

        process = pQueue_take(readyQueue);
        process->state = EXEC;

        pthread_mutex_lock(&mutex_log);
        log_info(kernelLogger, "El proceso %u se encuentra ahora ejecutandose", process->pid);
        pthread_mutex_unlock(&mutex_log);

        int contador = 0;
        // int seguirAtendiendo = true;     UNUSED.

        processState state = CONTINUE;
        // bool exited = false;     UNUSED.

        int socket = process->socket;

        while(state == CONTINUE){
            contador += contador;
            packet = socket_getPacket(socket);
            state = petitionProcessHandler[packet->header](packet, process, memSock);
            destroyPacket(packet);
        }
        if(state == BLOCK){ //Cambie BLOCKED por BLOCK. Ahora los tipos son compatibles pero no se que tan correcto sea.
            process->estimator = config->alpha * contador + (1-config->alpha) * process->estimator;

            pthread_mutex_lock(&mutex_log);
            log_info(kernelLogger, "El proceso %u bloqueado tras rafaga de %u. El valor de su estimador es %f", process->pid, contador, process->estimator);
            pthread_mutex_unlock(&mutex_log);

        } 
        else if(state == EXIT) {
            process->state = TERMINATED;    //Posiblemente innecesario

            pthread_mutex_lock(&mutex_log);
            log_info(kernelLogger, "Destruido el proceso %u (terminado)", process->pid);
            pthread_mutex_unlock(&mutex_log);

            destroyProcess(process);
        }
        //TODO: Implementar exit
    }
}

bool compareSJF(t_process* p1, t_process* p2){
    return p1->estimator < p2->estimator;
}

void updateWaited(t_process* process) {
    struct timespec start = process->startTime, end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    process->waited = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}

int responseRatio(t_process* process){
    updateWaited(process);
    return (process->estimator + process->waited) / process->estimator;
}

bool compareHRRN(t_process* p1, t_process* p2) {
    return responseRatio(p1) > responseRatio(p2);
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
            log_info(logger, "Dispositivo IO %s: el proceso %u pasa a ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);

            putToReady(process);
        }
        else if(process->state == SUSP_BLOCKED){
            pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pthread_mutex_unlock(&mutex_mediumTerm);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Dispositivo IO %s: el proceso %u pasa a suspended ready", self->nombre, process->pid);
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
            log_info(logger, "Semaforo %s: el proceso %u pasa a ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
            putToReady(process);
        }
        else if(process->state == SUSP_BLOCKED){
            pthread_mutex_lock(&mutex_mediumTerm);
            pQueue_removeBy(suspendedBlockedQueue, matchesPid);
            pthread_mutex_unlock(&mutex_mediumTerm);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Semaforo %s: el proceso %u pasa a suspended ready", self->nombre, process->pid);
            pthread_mutex_unlock(&mutex_log);
            pQueue_put(suspendedReadyQueue, (void*)process);
            sem_post(&sem_newProcess);
        }

        DDSemAllocated(dd, process, self);
    }
}

void *deadlockDetector_thread(void* args){
    t_deadlockDetector* self = (t_deadlockDetector*)args;

    int memorySocket = connectToServer(config->memoryIP, config->memoryPort);

    while(1){
        milliSleep(config->deadlockTime);
        pthread_mutex_lock(&deadlockDetector->mutex);
        while(findDeadlocks(self, memorySocket));
        pthread_mutex_unlock(&deadlockDetector->mutex);
    }
}