#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include "memoryConfig.h"
#include "networking.h"
#include "swapInterface.h"

t_swapInterface* swapInterface;
void* auxHandler(void *vclientSocket);

/* TO-DO:
    * Chequear que la logica de memwrite sea correcta.
*/

typedef struct heapMetadata { 
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isFree;
 } t_heapMetadata;

typedef struct mem {
    void *memory;
    t_memoryConfig *config;
} t_memory;

typedef struct frameMetadata {
    bool isFree;
    uint32_t PID;
    uint32_t page;
} t_frameMetadata;

typedef struct memoryMetadata{
    uint32_t entryQty;
    t_frameMetadata **entries;
} t_memoryMetadata;

extern bool (*petitionHandlers[MAX_PETITIONS])(t_packet* petition, int socket);

t_log *logger;
t_memory *ram;
t_dictionary *pageTables;
t_memoryConfig *config;
t_memoryMetadata *metadata;


t_memory *initializeMemory(t_memoryConfig *config);

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config);

void *petitionHandler(void *_clientSocket);

void destroyMemoryMetadata(t_memoryMetadata *metadata);

void memread(uint32_t bytes, uint32_t address, int PID, void *destination);

void memwrite(uint32_t bytes, uint32_t address, int PID, void *from);

int32_t getFreeFrame(t_memoryMetadata *memMetadata);

int32_t getPage(uint32_t address, t_memoryConfig *cfg);

int32_t getOffset(uint32_t address, t_memoryConfig *cfg);

// Esto no existe.
int32_t getFrame(uint32_t PID, uint32_t page);

void fijo(int32_t *start, int32_t *end, uint32_t PID);

void global(int32_t *start, int32_t *end, uint32_t PID);

void *ram_getFrame(t_memory* ram, uint32_t frameN);

#endif // !MEMORY_H_

