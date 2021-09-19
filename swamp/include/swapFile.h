#ifndef SWAPFILE_H_
#define SWAPFILE_H_

#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <commons/collections/list.h>

typedef struct swapFile{
    char* path;
    int fd;
    size_t size;
    size_t pageSize;
    int maxPages;
    t_list* entries;
} t_swapFile;

t_swapFile* swapFile_create(char* path, size_t size, size_t pageSize);

void swapFile_destroy(t_swapFile* self);

void swapFile_clearAtIndex(t_swapFile* sf, int index);

void* swapFile_readAtIndex(t_swapFile* sf, int index);

void* swapFile_writeAtIndex(t_swapFile* sf, int index, void* pagePtr);

bool swapFile_isFull(t_swapFile* sf);

bool swapFile_hasProcess(t_swapFile* sf);

int swapFile_countPagesOfProcess(t_swapFile* sf, uint32_t pid);

#endif // !SWAPFILE_H_
