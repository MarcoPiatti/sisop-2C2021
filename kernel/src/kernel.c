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
    thread_Cpus = calloc(config->multiprocess, sizeof(pthread_t));
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
    }
}

void *shortTerm_run(void* args) {
    t_process* process;
    while(1) {
        //TODO: implementar replanificación de forma tal que tanto el estimator como el waited se actualicen
        if(config->algorithm == "SJF")
            pQueue_sort(readyQueue, compareSJF);
        else                                        //Se asume que no hay otro algoritmo
            pQueue_sort(readyQueue, compareHRRN);   
    }
}

void *cpu(void* args) {
    t_process* process;
    while(1) {
        process = pQueue_take(readyQueue);//por ahora es fifo para mas palcer
        process->state = EXEC;

        int contador = 0;
        int seguirAtendiendo = true;
        
        while(seguirAtendiendo){
            contador += contador;
        
        
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

bool compareSJF(t_process* p1, t_process* p2){
    return p1->estimator < p2->estimator;
}

int responseRatio(t_process* process){
    return (process->estimator + process->waited) / process->estimator;
}

bool compareHRRN(t_process* p1, t_process* p2) {
    return responseRatio(p1) > responseRatio(p2);
}




//TODO: Agregar funcion CPU que reciba un proceso de ready, revise los paquetes y responda de manera acorde.