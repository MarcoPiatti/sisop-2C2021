#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include "utils.h"
#include "swapInterface.h"
#include <stdint.h>
#include <commons/string.h>

int main(){

    // Initialize logger.
    logger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
    
    // Load and validate config
    config = getMemoryConfig("./memory.cfg");
    validateConfg(config, logger);
    
    ram = initializeMemory(config);
    metadata = initializeMemoryMetadata(config);
    pageTables = dictionary_create();
    
    // De donde saca la memoria el puerto e IP del swap??
    // swapInterface = swapInterface_create(/*Swap IP */, /*swap port */, config->pageSize, /*swapheader algorithm */);

    int serverSocket = createListenServer(config->ip, config->port);
    
    while(1){
        runListenServer(serverSocket, petitionHandler);
    }

    return 0;
}

void *petitionHandler(void *_clientSocket){
    uint32_t clientSocket = (int*) _clientSocket;
    t_packet *petition = socket_getPacket(clientSocket);
    petitionHandlers[petition->header](petition, clientSocket);
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
    memcpy(aux, getFrameAddress(ram, firstFrame), toRead);
    aux += toRead;

    // Leer frames del medio completos.
    toRead = config->pageSize;
    size_t i;
    for (i = 1; i < fullPages; i++){
        readFrame(ram, getFrame(PID, firstPage + i), aux);
        aux += toRead;
    }

    // Leer "pedacito" al inicio de ultima pagina.
    toRead = bytes - toRead;
    memcpy(aux, getFrameAddress(ram, getFrame(PID, firstPage + i)), toRead);
    
}

void memwrite(uint32_t bytes, uint32_t address, int PID, void *from){
    uint32_t firstPage = getPage(address, config);
    uint32_t offset = getOffset(address, config);

    uint32_t toWrite = min(bytes, config->pageSize - offset);
    uint32_t fullPages = (bytes - toWrite) / config->pageSize;

    uint32_t firstFrame = getFrame(PID, firstPage);
    void *aux = from;

    // Escribir "pedacito" de memoria en el final de una pag.
    memcpy(getFrameAddress(ram, firstFrame), aux, toWrite);
    aux += toWrite;

    // Escribir frames del medio completos.
    toWrite = config->pageSize;
    size_t i;
    for (i = 1; i < fullPages; i++){
        writeFrame(ram, getFrame(PID, firstPage + 1), aux);
        aux += toWrite;
    }

    // Escribir "pedacito" al inicio de ultima pagina.
    toWrite = bytes - toWrite;
    memcpy(getFrameAddress(ram, getFrame(PID, firstPage + i)), aux, toWrite);
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

void fijo(int32_t *start, int32_t *end, uint32_t PID){
    *start = getFrame(PID, 0);
    *end = *start + config->framesPerProcess;
}

void global(int32_t *start, int32_t *end, uint32_t PID){
    *start = 0;
    *end = config->frameQty;
}