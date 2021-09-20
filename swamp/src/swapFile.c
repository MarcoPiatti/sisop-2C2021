#include "swapFile.h"

#include <commons/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

typedef struct pageMetadata{
    uint32_t pid;
    int32_t pageNumber;
    int index;
} t_pageMetadata;

bool matchesIndex(t_pageMetadata* entry, int index){
    return entry->index == index;
}

bool matchesPid(t_pageMetadata* entry, uint32_t pid){
    return entry->pid == pid;
}

bool matchesPage(t_pageMetadata* entry, int32_t pageNumber){
    return entry->pageNumber == pageNumber;
}

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
    return pagePtr;
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

bool swapFile_hasRoom(t_swapFile* sf){
    return !swapFile_isFull(sf);
}

bool swapFile_hasProcess(t_swapFile* sf){
    bool samePid(void* elem){
        return matchesPid((t_pageMetadata*) elem, pid);
    };
    return list_any_satisfy(sf->entries, samePid);
}

int swapFile_countProcesses(t_swapFile* sf){
    t_list* foundPids = list_create();
    void addIfUnrepeated(void* entry){
        bool samePid(void* elem){
            return matchesPid((t_pageMetadata*) elem, ((t_pageMetadata*)entry)->pid);
        };
        if(!list_any_satisfy(sf->entries, samePid)) list_add(foundPids, entry);
    };
    list_iterate(sf->entries, addIfUnrepeated);
    int amountFound = list_size(foundPids);
    list_destroy(foundPids);
    return amountFound;
}

int swapFile_countPagesOfProcess(t_swapFile* sf, uint32_t pid){
    bool samePid(void* elem){
        return matchesPid((t_pageMetadata*) elem, pid);
    };
    return list_count_satisfying(sf->entries, samePid);
}

int swapFile_getIndex(t_swapFile* sf, uint32_t pid, int32_t pageNumber){
    bool samePidAndPage(void* elem){
        return matchesPid((t_pageMetadata*) elem, pid) && matchesPage((t_pageMetadata*) elem), pageNumber);
    };
    t_pageMetadata* value = list_find(sf->entries, samePidAndPage);
    return value ? value->index : -1;
}

int swapFile_findFreeIndex(t_swapFile* sf){
    int i = 0;

    bool differentPid(void* elem){
        return !matchesPid((t_swapFile*)elem, i);
    }

    while(i < sf->maxPages){
        if(list_all_satisfy(sf->entries, differentPid)) break;
        i++;
    }

    return i;
}

void swapFile_register(t_swapFile* sf, uint32_t pid, int32_t pageNumber, int index){
    t_pageMetadata* newEntry = malloc(sizeof(t_pageMetadata));
    newEntry->index = index;
    newEntry->pageNumber = pageNumber;
    newEntry->pid = pid;
    list_add(sf->entries, (void*)newEntry);
}