#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include <commons/string.h>

void main(void){
    memoryLogger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
    memory = initializeMemory("./memory.cfg");
    pageTables = dictionary_create()
    validateConfg(memory->config, memoryLogger);
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

t_memory *initializeMemory(char *path){
    t_memory *newMemory = malloc(sizeof(t_memory));

    newMemory->config = getMemoryConfig(path);

    newMemory->metadata = initializeMemoryMetadata(newMemory->config);

    newMemory->memory = calloc(1, newMemory->config->size);
}

void memread(uint32_t bytes, uint32_t address, int PID, void *destination){
    
    // t_pageTable *pt = dictionary_get(string_from_format("%i", PID));
    // int offset = address % memory->config->pageSize;
    // int physical = toPhysical(PID, adress);
    // void *real = memory->memory + physical;

    // int toRead = min(bytes, memory->config->pageSize - (address % memory->config->pageSize));
    // memcpy(destination, real, toRead);

    // if (bytes != 0 ) memread(bytes - toRead, address + toRead, PID, destination + toRead);

}

void memwrite(uint32_t bytes, uint32_t address, int PID, void *from){
    ;
}
