#ifndef SWAPFILE_H_
#define SWAPFILE_H_

#include <unistd.h>
#include <sys/types.h>

typedef struct swapFile{
    char* path;
    int fd;
    size_t size;
    size_t pageSize;
    int    maxPages;
} t_swapFile;

t_swapFile* swapFile_create(char* path, size_t size, size_t pageSize);

void swapFile_destroy(t_swapFile* self);

void swapFile_cleanAll(t_swapFile* sf);

void swapFile_cleanAtIndex(t_swapFile* sf, int index);

void* swapFile_readAtIndex(t_swapFile* sf, int index);

void* swapFile_writeAtIndex(t_swapFile* sf, int index, void* pagePtr);

#endif // !SWAPFILE_H_
