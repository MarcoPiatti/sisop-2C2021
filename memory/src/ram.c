#include "ram.h"

#include <stdlib.h>
#include <string.h>

t_ram* createRam(int size, int pageSize, int(*victimAlgorithm)(t_ram*, int, int), void(*assignmentType)(t_ram*, uint32_t, int*, int*), int maxFrames){
    t_ram* self = malloc(sizeof(t_ram));
    self->size = size;
    self->pageSize = pageSize;
    self->frames = self->size / self->pageSize;
    self->data = malloc(self->size);
    self->metadata = malloc(sizeof(t_frameMetadata) * self->frames);
    
    for(int i = 0; i < self->frames; i++)
        self->metadata[i].isFree = true;

    self->LRU_clock = 0;
    self->victimAlgorithm = victimAlgorithm;
    self->assignmentType = assignmentType;
    self->maxFrames = maxFrames;
    return self;
}

void destroyRam(t_ram* self){
    free(self->metadata);
    free(self->data);
    free(self);
}

void fixed(t_ram* self, uint32_t pid, int* lowerBound, int* upperBound){
    *lowerBound = -2;
    *upperBound = -2;
    for(int i = 0; i < self->frames; i += self->maxFrames)
        if(self->metadata[i].pid == pid){
            *lowerBound = i;
            *upperBound = self->maxFrames + *lowerBound;
            return;
        }
    for(int i = 0; i < self->frames; i += self->maxFrames)
        if(self->metadata[i].isFree){
            *lowerBound = i;
            *upperBound = self->maxFrames + *lowerBound;
            return;
        }
    return;
}

void global(t_ram* self, uint32_t pid, int* lowerBound, int* upperBound){
    *lowerBound = 0;
    *upperBound = self->frames;
}

int ram_findFreeFrame(t_ram* self, uint32_t pid){
    int lowerBound, upperBound;
    self->assignmentType(self, pid, &lowerBound, &upperBound);
    
    if(lowerBound == -2) return -2;

    int found = -1;
    for(int i = lowerBound; i < upperBound; i++)
        if(self->metadata[i].isFree){
            found = i;
            break;
        }
    return found;
}

int ram_countFreeFrames(t_ram* self, uint32_t pid){
    int lowerBound, upperBound;
    self->assignmentType(self, pid, &lowerBound, &upperBound);

    if(lowerBound == -2) return -2;

    int found = 0;
    for(int i = lowerBound; i < upperBound; i++)
        if(self->metadata[i].isFree){
            found++;
        }
    return found;
}

void* ram_getFrame(t_ram* self, int index){    
    self->metadata[index].lastUsed = ++self->LRU_clock;
    self->metadata[index].chance = true;
    return self->data + index * self->pageSize;
}

void ram_replaceFrame(t_ram* self, int index, void* data){
    memcpy(self->data + index * self->pageSize, data, self->pageSize);
    self->metadata[index].lastUsed = ++self->LRU_clock;
    self->metadata[index].chance = true;
}

void ram_editFrame(t_ram* self, int index, int offset, void* data, int size){
    memcpy(self->data + index * self->pageSize + offset, data, size);
    self->metadata[index].lastUsed = ++self->LRU_clock;
    self->metadata[index].chance = true;
    self->metadata[index].modified = true;
}

t_frameMetadata* ram_getFrameMetadata(t_ram* self, int index){
    return self->metadata + index;
}

void ram_replaceFrameMetadata(t_ram* self, int index, uint32_t pid, int32_t page){
    self->metadata[index].isFree = false;
    self->metadata[index].pid = pid;
    self->metadata[index].page = page;
    self->metadata[index].chance = true;
    self->metadata[index].modified = false;
    self->metadata[index].lastUsed = ++self->LRU_clock;
}

void ram_clearFrameMetadata(t_ram* self, int index){
    self->metadata[index].isFree = true;
}

int ram_getVictimIndex(t_ram* self, uint32_t pid){
    int lowerBound, upperBound;
    self->assignmentType(self, pid, &lowerBound, &upperBound);
    int victimIndex = self->victimAlgorithm(self, lowerBound, upperBound); 
    return victimIndex;
}

int LRU(t_ram* self, int lowerBound, int upperBound){
    int oldest = lowerBound;
    for (int i = lowerBound; i < upperBound; i++)
        if(self->metadata[oldest].lastUsed > self->metadata[i].lastUsed)
            oldest = self->metadata[i].lastUsed;
    return oldest;
}

int CLOCK_M(t_ram* self, int lowerBound, int upperBound){
    int victim = -1;

    while(victim == -1){
        for (int i = lowerBound; i < upperBound; i++)
            if(!self->metadata[i].chance && !self->metadata[i].modified){
                victim = i;
                return victim;
            }

        for (int i = lowerBound; i < upperBound; i++){
            if(!self->metadata[i].chance && self->metadata[i].modified){
                victim = i;
                return victim;
            }
            self->metadata[i].chance = false;
        }
    }

    return victim;
}