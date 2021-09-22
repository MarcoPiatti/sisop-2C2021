#include "swapFile.h"

#include <commons/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

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

    self->entries = malloc(sizeof(t_pageMetadata) * self->maxPages);
    for(int i = 0; i < self->maxPages; i++)
        self->entries[i].used = false;

    return self;
}

void swapFile_destroy(t_swapFile* self){
    close(self->fd);
    free(self->path);
    free(self->entries);
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
    void* pagePtr = malloc(sizeof(sf->pageSize));
    memcpy(pagePtr, mappedFile + index * sf->pageSize, sf->pageSize);
    munmap(mappedFile, sf->size);
    return pagePtr;
}

void swapFile_writeAtIndex(t_swapFile* sf, int index, void* pagePtr){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE,MAP_SHARED, sf->fd, 0);
    memcpy(mappedFile + index * sf->pageSize, pagePtr, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

bool swapFile_isFull(t_swapFile* sf){
    return !swapFile_hasRoom(sf);
}

bool swapFile_hasRoom(t_swapFile* sf){
    bool hasRoom = false;
    for(int i = 0; i < sf->maxPages; i++)
        if(!sf->entries[i].used){
            hasRoom = true;
            break;
        }
    return hasRoom;
}

bool swapFile_hasPid(t_swapFile* sf, uint32_t pid){
    bool hasPid = false;
    for(int i = 0; i < sf->maxPages; i++)
        if(sf->entries[i].used && sf->entries[i].pid == pid){
            hasPid = true;
            break;
        }
    return hasPid;
}

int swapFile_countPidPages(t_swapFile* sf, uint32_t pid){
    int pages = 0;
    for(int i = 0; i < sf->maxPages; i++)
        if(sf->entries[i].used && sf->entries[i].pid == pid)
            pages++;
    return pages;
}

bool swapFile_isFreeIndex(t_swapFile* sf, int index){
    return sf->entries[index].used;
}

int swapFile_getIndex(t_swapFile* sf, uint32_t pid, int32_t pageNumber){
    int i = 0;
    while(sf->entries[i].pid != pid || sf->entries[i].pageNumber != pageNumber){
        if(i >= sf->maxPages) return -1;
        i++;
    }
    return i;
}

int swapFile_findFreeIndex(t_swapFile* sf){
    int i = 0;
    while(!sf->entries[i].used){
        if(i >= sf->maxPages) return -1;
        i++;
    }
    return i;
}

void swapFile_register(t_swapFile* sf, uint32_t pid, int32_t pageNumber, int index){
    sf->entries[index].used = true;
    sf->entries[index].pid = pid;
    sf->entries[index].pageNumber = pageNumber;
}