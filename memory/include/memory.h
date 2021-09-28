#ifndef MEMORY_H_
#define MEMORY_H_

#include "networking.h"
#include "swapInterface.h"
#include "memoryConfig.h"
#include "ram.h"
#include "tlb.h"
#include "pageTable.h"
#include "logs.h"
#include <string.h>
#include <unistd.h>

typedef struct HeapMetadata {
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isFree;
} t_HeapMetadata;


t_memoryConfig* memoryConfig;

t_swapInterface* swapInterface;

t_TLB* tlb;

t_ram* ram;

t_pageTable* pageTable;

//TODO: Implementar algun tipo de sincronizacion entre las TADs de ram, pageTable y TLB o por afuera.
//Por ahora lo dejo modo croto con un mutex gigante que acapara toda la operacion de un saque.
pthread_mutex_t mutexMemoria;

extern bool (*petitionHandlers[MAX_PETITIONS])(int clientSocket, t_packet* petition);

#endif // !MEMORY_H_