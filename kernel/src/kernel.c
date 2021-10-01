#include "kernel.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"


t_log *kernelLogger;
sem_t cuposDisponibles;

void main(void){
    kernelLogger = log_create("./kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    config = getKernelConfig("./kernel.cfg");

    sem_init(&cuposDisponibles, 0, config->multiprogram);

    pthread_create(thread_longTerm, NULL, longTerm_run, NULL);
    pthread_detach(thread_longTerm);

    int serverSocket = createListenServer(config->ip, config->port);

    runListenServer(serverSocket, auxHandler);

    close(serverSocket);
}

void *auxHandler(void *vclientSocket){
    int clientSocket = (int*) vclientSocket;
    socket_sendHeader(clientSocket, OK);
    
    t_packet *packet;
    t_process* proceso;
    
    uint32_t idCliente;

    socket_sendHeader(clientSocket, ID_KERNEL);

    packet = socket_getPacket(clientSocket);
    idCliente = streamTake_UINT32(packet->payload);
    proceso = createProcess(idCliente, clientSocket, config->initialEstimation);
    pQueue_put(newQueue, proceso);
    destroyPacket(packet);
}

void *longTerm_run(void* args) {
    t_process* proceso;
    while(1) {
        sem_wait(&cuposDisponibles);
        proceso = pQueue_take(newQueue);
        pQueue_put(readyQueue, proceso);
    }
}