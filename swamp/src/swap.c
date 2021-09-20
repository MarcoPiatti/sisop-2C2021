#include "swap.h"

t_swapFile* pidExists(uint32_t pid){
    bool hasProcess(void* elem){
        return swapFile_hasPid((t_swapFile*)elem, pid);
    };
    t_swapFile* file = list_find(swapFiles, hasProcess);
    return file;
}

bool hasFreeChunk(t_swapFile* sf){
    bool hasFreeChunk = false;
    for(int i = 0; i < sf->maxPages; i += swapConfig->maxFrames)
        if(!sf->entries[i].used){
            hasFreeChunk = true;
            break;
        }
    return hasFreeChunk;
}

int findFreeChunk(t_swapFile* sf){
    int i = 0;
    while(!sf->entries[i].used){
        if(i >= sf->maxPages) return -1;
        i += swapConfig->maxFrames;
    }
    return i;
}

int getChunk(t_swapFile* sf, uint32_t pid){
    int i = 0;
    while(sf->entries[i].pid != pid){
        if(i >= sf->maxPages) return -1;
        i += swapConfig->maxFrames;
    }
    if(!sf->entries[i].used) return -1;
    return i;
}

bool fija(uint32_t pid, int32_t page, void* pageContent){
    t_swapFile* file = pidExists(pid);
    bool _hasFreeChunk(void* elem){
        return hasFreeChunk((t_swapFile*)elem);
    };
    if (file == NULL)
        file = list_find(swapFiles, _hasFreeChunk);
    if (file == NULL)
        return false;

    int base = getChunk(file, pid);
    if (base == -1) base = findFreeChunk(file);

    int offset = swapFile_countPidPages(file, pid);
    if (offset == swapConfig->maxFrames) return false;

    int assignedIndex = base * swapConfig->maxFrames + offset;
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    return true;
}

bool global(uint32_t pid, int32_t page, void* pageContent){
    t_swapFile* file = pidExists(pid);
    
    bool _hasRoom(void* elem){
        return swapFile_hasRoom((t_swapFile*)elem);
    };
    if (file == NULL)
        file = list_find(swapFiles, _hasRoom);
    if (file == NULL)
        return false;
    if (swapFile_isFull(file))
        return false;
    
    int assignedIndex = swapFile_findFreeIndex(file);
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    return true;
}

int main(){
    swapConfig = getswapConfig("./cfg/swap.config");

    for(int i = 0; swapConfig->swapFiles; i++)
        list_add(swapFiles, swapFile_create(swapConfig->swapFiles[i], swapConfig->fileSize, swapConfig->pageSize));

    int serverSocket = createListenServer(swapConfig->swapIP, swapConfig->swapPort);
    int memorySocket = getNewClient(serverSocket);
    swapHeader asignType = socket_getHeader(memorySocket);
    if (asignType == ASIGN_FIJO) asignacion = fija;
    else if (asignType == ASIGN_GLOBAL) asignacion = global;

    t_packet* petition;
    while(1){
        petition = socket_getPacket(memorySocket);
        for(int i = 0; i < swapConfig->delay; i++){
            usleep(1000);
        }
        swapHandler[petition->header](petition, memorySocket);
        destroyPacket(petition);
    }

    return 0;
}