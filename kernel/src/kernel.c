#include "kernel.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include "kernelConfig.h"

t_log *kernelLogger;

void main(void){
    kernelLogger = log_create("./kernel.log", "KERNEL", 1, LOG_LEVEL_TRACE);

    t_kernelConfig* config = getKernelConfig("./kernel.cfg");

    int serverSocket = createListenServer(config->ip, config->port);

    runListenServer(serverSocket, auxHandler);

    close(serverSocket);
}

void *auxHandler(void *vclientSocket){
    int clientSocket = (int*) vclientSocket;
    socket_sendHeader(clientSocket, OK);
    
    t_packet *packet;
    int header = 0;

    do{
        //TODO: Tener respuesta para cada tipo de mensaje
        packet = socket_getPacket(clientSocket);
        header = packet->header;
        destroyPacket(packet);
        log_info(kernelLogger, "Header de paquete recibido: %i", header);
        socket_sendHeader(clientSocket, OK);
        log_info(kernelLogger, "Enviado OK");
    } while (header != DISCONNECTED);
}
