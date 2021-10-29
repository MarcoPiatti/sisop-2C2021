#include "kernel.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"


t_log *kernelLogger;
sem_t cuposDisponibles, availableCPUS, runShortTerm;


void main(void){
    kernelLogger = log_create("./kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    config = getKernelConfig("./kernel.cfg");

    //Inicializo estructura t_processQueues
    processQueues.newQueue = newQueue;
    processQueues.readyQueue = readyQueue;
    processQueues.blockedQueue = blockedQueue;
    processQueues.suspendedReadyQueue = suspendedReadyQueue;
    processQueues.suspendedBlockedQueue = suspendedBlockedQueue;
    processQueues.execQueue = execQueue;

    //Inicializo diccionario de mateSems
    mateSems = dictionary_create();

    //Inicializacion semáforos
    sem_init(&cuposDisponibles, 0, config->multiprogram);
    sem_init(&availableCPUS, 0, config->multiprocess);
    sem_init(&runShortTerm, 0, 0);     //Semáforo para correr el short term scheduler solo cuando un proceso llega a ready
    
    pthread_create(thread_longTerm, NULL, longTerm_run, NULL);
    pthread_detach(thread_longTerm);
    thread_Cpus = calloc(config->multiprocess, sizeof(pthread_t));
    for(int i=0;i<config->multiprocess;i++){
        pthread_create(thread_Cpus[i], NULL, cpu, NULL);
    }

    int serverSocket = createListenServer(config->ip, config->port);

    while(1){   //TODO: Chequear
        runListenServer(serverSocket, auxHandler);
    }

    close(serverSocket);
}

void *auxHandler(void *vclientSocket){
    int clientSocket = (int*) vclientSocket;
    socket_sendHeader(clientSocket, OK);
    
    t_packet *packet;
    t_process* process;
    
    uint32_t idCliente;

    socket_sendHeader(clientSocket, ID_KERNEL);

    packet = socket_getPacket(clientSocket);
    idCliente = streamTake_UINT32(packet->payload);
    process = createProcess(idCliente, clientSocket, config->initialEstimation);
    pQueue_put(newQueue, process);
    destroyPacket(packet);
}

void *longTerm_run(void* args) {
    t_process* process;
    while(1) {
        sem_wait(&cuposDisponibles);
        process = pQueue_take(newQueue);
        process->state = READY;
        pQueue_put(readyQueue, process);
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
    while(1) {
        sem_wait(&runShortTerm);
        pQueue_iterate(readyQueue, updateWaited);
        if(config->algorithm == "SJF")
            pQueue_sort(readyQueue, compareSJF);
        else                                        //Se asume que no hay otro algoritmo
            pQueue_sort(readyQueue, compareHRRN);   
    }
}

void updateWaited(t_process* process){
    process->waited += 1; 
}

void *cpu(void* args) {
    t_process* process;
    t_packet* packet;
    while(1) {
        
        process = pQueue_take(readyQueue);
        process->state = EXEC;

        int contador = 0;
        int seguirAtendiendo = true;

        processState state = CONTINUE;
        bool exited = false;

        int socket = process->socket;
        
        while(state == CONTINUE){
            contador += contador;
            packet = socket_getPacket(socket);
            state = petitionProcessHandler[packet->header](packet, socket);
            //TODO: Verificar exit
            destroyPacket(packet);
        }
        if(state==BLOCKED){
            process->estimator = config->alpha *contador+(1-config->alpha)* process->estimator;
        }
        //TODO: Implementar exit
    }
}




bool compareSJF(t_process* p1, t_process* p2){
    return p1->estimator < p2->estimator;
}

void updateWaited(t_process* process) {
    struct timespec start = process->startTime, end;
    clock_getTime(CLOCK_MONOTONIC, &end);
    process->waited = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}

int responseRatio(t_process* process){
    updateWaited(process);
    return (process->estimator + process->waited) / process->estimator;
}

bool compareHRRN(t_process* p1, t_process* p2) {
    return responseRatio(p1) > responseRatio(p2);
}




//TODO: Agregar funcion CPU que reciba un proceso de ready, revise los paquetes y responda de manera acorde. //Ojo quizas este "//TODO:" ya esta obsoleto