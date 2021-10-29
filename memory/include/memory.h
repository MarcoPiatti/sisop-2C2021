#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include "memoryConfig.h"
#include "networking.h"

void* auxHandler(void *vclientSocket);

typedef​ ​struct​ ​heapMetadata​ { 
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isFree;
} t_heapMetadata;

typedef struct mem {
    void *memory;
} t_memory;

typedef struct memoryMetadata{
    uint32_t entryQty;
    t_frameMetadata **entries;
} t_memoryMetadata;

typedef struct frameMetadata {
    bool isFree;
    uint32_t PID;
    uint32_t page;
} t_frameMetadata;

extern bool (*petitionHandlers[MAX_PETITIONS])(t_packet* petition, int socket);

t_log *logger;
t_memory *memory;
t_dictionary *pageTables;
t_memoryConfig *config;
t_memoryMetadata *metadata;

t_memory *initializeMemory(t_memoryConfig *config);

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config);

void destroyMemoryMetadata(t_memoryMetadata *metadata);

void memread(uint32_t bytes, uint32_t address, int PID, void *destination);

void memwrite(uint32_t bytes, uint32_t address, int PID, void *from);

int32_t getFreeFrame(t_memoryMetadata *memMetadata);


#endif // !MEMORY_H_

