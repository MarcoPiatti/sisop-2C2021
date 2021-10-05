#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>

void *auxHandler(void *vclientSocket);

typedef​ ​struct​ ​heapMetadata​ { 
    uint32_t prevAlloc,
    uint32_t nextAlloc,
    uint8_t isFree,
} t_heapMetadata;



#endif // !MEMORY_H_

