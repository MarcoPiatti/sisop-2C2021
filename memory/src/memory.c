#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"

t_log *memoryLogger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);

void main(void){
    t_config *config = config_create("./memory.cfg");
    char *port = config_get_string_value(config, "PORT");
    char *ip = config_get_string_value(config, "IP"); 

    int serverSocket = createListenServer(ip, port);

    runListenServer(serverSocket, auxHandler);

    close(serverSocket);
}

void *auxHandler(void *vclientSocket){
    int clientSocket = (int*) vclientSocket;
    socket_sendHeader(clientSocket, OK);
    
    t_packet *packet;
    int header = 0;

    do{
        packet = socket_getPacket(clientSocket);
        header = packet->header;
        destroyPacket(packet);
        log_info(memoryLogger, "Header de paquete recibido: %i", header);
        socket_sendHeader(clientSocket, OK);
        log_info(memoryLogger, "Enviado OK");
    } while (header != DISCONNECTED);
}