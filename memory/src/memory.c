#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include <commons/string.h>

int main(){
    logger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
    
    config = getMemoryConfig("./memory.cfg");
    validateConfg(config, logger);
    
    memory = initializeMemory(config);
    metadata = initializeMemoryMetadata(config);
    pageTables = dictionary_create();
    

    int serverSocket = createListenServer(config->ip, config->port);
    while(1){
        runListenServer(serverSocket, auxHandler);
    }

    return 0;
}

void *auxHandler(void *vclientSocket){
    int clientSocket = (int*) vclientSocket;
    socket_sendHeader(clientSocket, ID_MEMORIA);
    
    t_packet *packet;
    int header = 0;

    do{
        packet = socket_getPacket(clientSocket);

    } while (header != DISCONNECTED);
}

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config){
    t_memoryMetadata *newMetadata = malloc(sizeof(t_memoryMetadata));
    newMetadata->entryQty = config->size / config->pageSize;
    newMetadata->entries = calloc(newMetadata->entryQty, sizeof(t_frameMetadata));

    for (int i = 0; i < newMetadata->entryQty; i++){
        ((newMetadata->entries)[i])->isFree = 1;
    }

    return newMetadata; 
}

void destroyMemoryMetadata(t_memoryMetadata *metadata){
    free(metadata->entries);
    free(metadata);
}

t_memory *initializeMemory(t_memoryConfig *config){
    t_memory *newMemory = malloc(sizeof(t_memory));
    newMemory->memory = calloc(1,config->size);

    return newMemory;
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

int32_t getFreeFrame(t_memoryMetadata *memMetadata){
    for (int i = 0; i < memMetadata->entryQty; i++){
        if (((memMetadata->entries)[i])->isFree == 1) return i;
    }

    return -1;
}