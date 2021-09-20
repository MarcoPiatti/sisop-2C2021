#ifndef SWAP_H_
#define SWAP_H_

#include "swapConfig.h"
#include <commons/collections/list.h>

t_swapConfig* swapConfig;
t_list* swapFiles;

bool (*asignacion)(uint32_t pid, int32_t page, void* pageContent);

#endif // !SWAP_H_