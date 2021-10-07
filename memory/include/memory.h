#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include "memoryConfig.h"

void *auxHandler(void *vclientSocket);

typedef​ ​struct​ ​heapMetadata​ { 
    uint32_t prevAlloc,
    uint32_t nextAlloc,
    uint8_t isFree,
} t_heapMetadata;

typedef struct pageTableEntry{
    bool present;
    uint32_t frame;
}t_pageTableEntry;

typedef struct pageTable{
    int pageQuantity;
    t_pageTableEntry **entries;
}t_pageTable;

typedef struct memoryMetadata{
    // Bitmap que contiene el estado de los frames (libre/ocupado).
    bool **entries;
}t_memoryMetadata;

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config);

void destroyMemoryMetadata(t_memoryMetadata *metadata);

t_pageTable *initializePageTable(t_memoryConfig *config);

void destroyPageTable(t_pageTable *table);

void pageTableAddEntry(t_pageTable *table, uint32_t newFrame);

#endif // !MEMORY_H_

