#ifndef SWAP_H_
#define SWAP_H_

#include "swapConfig.h"
#include <commons/collections/list.h>
#include <stdint.h>
#include "networking.h"
#include "headers.h"
#include "swapFile.h"

t_swapConfig* swapConfig;
t_list* swapFiles;

t_swapFile* pidExists(uint32_t pid);

bool (*asignacion)(uint32_t pid, int32_t page, void* pageContent);

extern void (*swapHandler[MAX_MEM_MSGS])(t_packet* petition, int memorySocket);

#endif // !SWAP_H_