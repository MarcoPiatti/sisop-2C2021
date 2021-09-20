#include "swap.h"
#include "networking.h"
#include "swapFile.h"

t_swapFile* pidExists(uint32_t pid){
    bool hasProcess(void* elem){
        return swapFile_hasProcess((t_swapFile*)elem);
    };
    t_swapFile* file = list_find(swapFiles, hasProcess);
    return file;
}

bool asignacionFija(uint32_t pid, int32_t page, void* pageContent){
    int maxProcesses = swapConfig->fileSize / (swapConfig->maxFrames * swapConfig->pageSize);
    t_swapFile* file = pidExists(pid);
    bool _hasRoom(void* elem){
        return swapFile_countProcesses((t_swapFile*)elem) < maxProcesses;
    };
    if (file == NULL)
        file = list_find(swapFiles, _hasRoom);
    if (file == NULL)
        return false;
    if (swapFile_countPagesOfProcess(file, pid) == swapConfig->maxFrames)
        return false;

    int assignedIndex = swapFile_findFreeIndex(file);
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    return true;
}

bool asignacionGlobal(uint32_t pid, int32_t page, void* pageContent){
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

    for(int i = 0; swapConfig->swapFiles; i++){
        list_add(swapFiles, swapFile_create(swapConfig->swapFiles[i], swapConfig->fileSize, swapConfig->pageSize));
    }

    int serverSocket = createListenServer(swapConfig->swapIP, swapConfig->swapPort);
    int memorySocket = getNewClient(serverSocket);

    while(1){
        
    }

    return 0;
}