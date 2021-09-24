#ifndef MEMORY_H_
#define MEMORY_H_

#include "networking.h"
#include "swapInterface.h"
#include "memoryConfig.h"
#include "tlb.h"
#include <string.h>

t_memoryConfig* memoryConfig;

t_swapInterface* swapInterface;

void* ram;

t_TLB* tlb;

extern bool (*petitionHandlers[MAX_PETITIONS])(int clientSocket, t_packet* petition);

#endif // !MEMORY_H_