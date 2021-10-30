#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include <stdint.h>
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
    newMetadata->entryQty = config->frameQty;
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
    newMemory->config = config;

    return newMemory;
}

void memread(uint32_t bytes, uint32_t address, int PID, void *destination){
    uint32_t firstPage = getPage(address, config);
    uint32_t offset = getOffset(address, config);

    uint32_t toRead = min(bytes, config->pageSize - offset);
    uint32_t fullPages = (bytes - toRead) / config->pageSize;

    uint32_t firstFrame = getFrame(PID, firstPage);
    void *aux = destination;

    // Leer "pedacito" de memoria en el final de una pag.
    memcpy(aux, getFrameAddress(memory, firstFrame), toRead);
    aux += toRead;

    // Leer frames del medio completos.
    toRead = config->pageSize;
    size_t i;
    for (i = 1; i < fullPages; i++){
        readFrame(memory, getFrame(PID, firstPage + i), aux)
        aux += toRead
    }

    // Leer "pedacito" al inicio de ultima pagina.
    toRead = bytes - toRead;
    memcpy(aux, getFrameAddress(memory, getFrame(PID, firstPage + i)), toRead);
    
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

void readFrame(t_memory *mem, uint32_t frame, void *dest){
    void *frameAddress = getFrameAddress(mem, frame);
    memcpy(dest, frameAddress, mem->config->pageSize);
}

void writeFrame(t_memory *mem, uint32_t frame, void *from){
    void *frameAddress = getFrameAddress(mem, frame);
    memcpy(frameAddress, from, mem->config->pageSize);
}

void *getFrameAddress(t_memory *mem, uint32_t frame){
    return mem->memory + frame * config->pageSize;
}

int32_t getPage(uint32_t address, t_memoryConfig *cfg){
    return address / cfg->pageSize;
}

int32_t getOffset(uint32_t address, t_memoryConfig *cfg){
    return address % cfg->pageSize;
}

// Asumo la existencia de una funcion que, a partir del PID y el N de pag, devuelve el N de frame.
int32_t getFrame(uint32_t PID, uint32_t page){
    return 0;
}

int32_t min(int32_t a, int32_t b) {
    if (a < b) return a;
    return b;
}
