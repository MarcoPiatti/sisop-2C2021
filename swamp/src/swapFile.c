#include "swapFile.h"

#include <commons/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

typedef struct pageMetadata{
    uint32_t pid;
    int32_t pageNumber;
} t_pageMetadata;

t_swapFile* swapFile_create(char* path, size_t size, size_t pageSize){
    t_swapFile* self = malloc(sizeof(t_swapFile));
    self->path = string_duplicate(path);
    self->size = size;
    self->pageSize = pageSize;
    self->maxPages = size / pageSize;
    self->fd = open(self->path, O_RDWR|O_CREAT, S_IRWXU);
    ftruncate(self->fd, self->size);
    void* mappedFile = mmap(NULL, self->size, PROT_READ|PROT_WRITE,MAP_SHARED, self->fd, 0);
    memset(mappedFile, 0, self->size);
    msync(mappedFile, self->size, MS_SYNC);
    munmap(mappedFile, self->size);

    self->entries = list_create();
    for(int i = 0; i < self->maxPages; i++)
        list_add(self->entries, NULL);

    return self;
}

void swapFile_destroy(t_swapFile* self){
    close(self->fd);
    free(self->path);
    list_destroy_and_destroy_elements(self->entries, free);
    free(self);
}

void swapFile_clearAtIndex(t_swapFile* sf, int index){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE, MAP_SHARED, sf->fd, 0);
    memset(mappedFile + index * sf->pageSize, 0, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

void* swapFile_readAtIndex(t_swapFile* sf, int index){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE,MAP_SHARED, sf->fd, 0);
    void* pagePtr = NULL;
    memcpy(pagePtr, mappedFile + index * sf->pageSize, sf->pageSize);
    munmap(mappedFile, sf->size);
}

void swapFile_writeAtIndex(t_swapFile* sf, int index, void* pagePtr){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE,MAP_SHARED, sf->fd, 0);
    memcpy(mappedFile + index * sf->pageSize, pagePtr, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

bool swapFile_isFull(t_swapFile* sf){
    return list_size(sf->entries) == sf->maxPages;
}

int swapFile_countPagesOfProcess(t_swapFile* sf, uint32_t pid){
    bool samePid(void* elem){
        pid == ((t_pageMetadata*)elem)->pid;
    };
    return list_count_satisfying(sf->entries, samePid);
}