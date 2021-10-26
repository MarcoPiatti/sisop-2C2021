#include "kernel.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"


t_log *kernelLogger;
sem_t cuposDisponibles, availableCPUS;

void main(void){
    kernelLogger = log_create("./kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    config = getKernelConfig("./kernel.cfg");

    sem_init(&cuposDisponibles, 0, config->multiprogram);
    sem_init(&availableCPUS, 0, config->multiprocess);
    
    pthread_create(thread_longTerm, NULL, longTerm_run, NULL);
    pthread_detach(thread_longTerm);
    thread_Cpus = malloc(sizeof(pthread_t*config->multiprocess));
    for(int i=0;i<config->multiprocess;i++){
    pthread_create(thread_Cpus[i], NULL, cpu, NULL);
}

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
        proceso->state = READY;
        pQueue_put(readyQueue, proceso);
    }
}

void *shortTerm_run(void* args) {
    t_process* proceso;
    while(1) {
        pQueue_sort(readyQueue,compararSJF);
    }
}

void *cpu(void* args) {
    t_process* proceso;
    while(1) {
        proceso = pQueue_take(readyQueue);//por ahora es fifo para mas palcer
        proceso->state = EXEC;

        int contador = 0;
        int seguirAtendiendo = true;
        while(seguirAtendiendo){
        contador += contador;
        for(int i=0;i<2;i++){
            if
        }
        
        /*
        if(?){//no se como saber cuando estoy bloquiao
        proceso->state = BLOCKED;

        proceso->estimator = config->alpha *contador+(1-config->alpha)*proceso->estimator;
        pQueue_put(blockedQueue,proceso);
        break;
        }
 */
        }
    }
}

bool _sem_wait(t_packet *received, int clientSocket){
    return true;
}

bool _call_io(t_packet *received, int clientSocket){
    return true;
}

bool (*petitionProcessHandler[2])(t_packet *received, int clientSocket) = {
    _sem_wait,
    _call_io
};



bool compararSJF( t_process* p1, t_process* p2){
    return (p2->estimator>p1->estimator)?true:false;
}




//TODO: Agregar funcion CPU que reciba un proceso de ready, revise los paquetes y responda de manera acorde.