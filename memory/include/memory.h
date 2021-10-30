#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include "memoryConfig.h"
#include "networking.h"

void* auxHandler(void *vclientSocket);



/* TO-DO:
    * Mover min() a un archivo de utils generales en shared.
    * Unificar criterio de uso de globales vs pasar como parametro a funciones.
    * getFrame()
    * Chequear que la logica de memwrite sea correcta.
*/

typedef​ ​struct​ ​heapMetadata​ { 
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isFree;
} t_heapMetadata;

typedef struct mem {
    void *memory;
    t_memoryConfig *config;
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

int32_t getPage(uint32_t address, t_memoryConfig *cfg);

int32_t getOffset(uint32_t address, t_memoryConfig *cfg);

// Esto no existe.
int32_t getFrame(uint32_t PID, uint32_t page);

// Esto deberia ir an algun archivo de utils en shared.
int32_t min(int32_t a, int32_t b);

#endif // !MEMORY_H_

