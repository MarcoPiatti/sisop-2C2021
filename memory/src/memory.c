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

t_pageTable *initializePageTable(t_memoryConfig *config){
    t_pageTable *newTable = malloc(sizeof(t_pageTable));
    newTable->pageQuantity = 0;
    return newTable;
}

void destroyPageTable(t_pageTable *table){
    free(table->entries);
    free(table);
}

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config){
    t_memoryMetadata *newMetadata = malloc(sizeof(t_memoryMetadata));
    int pageQty = config->size / config->pageSize;
    newMetadata->entries = calloc(sizeof(bool), pageQty);
    memset(newMetadata->entries, 0, sizeof(bool) * pageQty);
    return newMetadata;
}

void destroyMemoryMetadata(t_memoryMetadata *metadata){
    free(metadata->entries);
    free(metadata);
}

void pageTableAddEntry(t_pageTable *table, uint32_t newFrame, bool isPresent){
    table->entries = realloc(table->entries, sizeof(t_pageTableEntry)*(table->pageQuantity + 1));
    (table->entries)[table->pageQuantity]->frame = newFrame;
    (table->entries)[table->pageQuantity]->present = isPresent;
    (table->pageQuantity)++;
}