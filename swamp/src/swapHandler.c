#include "swap.h"

void savePage(t_packet* petition, int memorySocket){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t pageNumber = streamTake_INT32(petition->payload);
    void* pageData = NULL;
    streamTake(petition->payload, &pageData, (size_t)swapConfig->pageSize);
    bool rc = asignacion(pid, pageNumber, pageData);
    t_packet* response;
    if(rc) response = createPacket(OK_MEM, 0);
    else response = createPacket(ERROR_MEM, 0);
    socket_sendPacket(memorySocket, response);
    destroyPacket(response);
}

void loadPage(t_packet* petition, int memorySocket){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t pageNumber = streamTake_INT32(petition->payload);
    t_swapFile* file = pidExists(pid);
    if(file == NULL){
        t_packet* response = createPacket(ERROR_MEM, 0);
        socket_sendPacket(memorySocket, response);
        destroyPacket(response);
        return;
    }
    int index = swapFile_getIndex(file, pid, pageNumber);
    if(index == -1){
        t_packet* response = createPacket(ERROR_MEM, 0);
        socket_sendPacket(memorySocket, response);
        destroyPacket(response);
        return;
    }
    void* pageData = swapFile_readAtIndex(file, index);

    t_packet* response = createPacket(PAGE_DATA, (size_t)swapConfig->pageSize);
    streamAdd(response->payload, pageData, swapConfig->pageSize);
    socket_sendPacket(memorySocket, response);
    destroyPacket(response);
}

void eraseProcess(t_packet* petition, int memorySocket){
    uint32_t pid = streamTake_UINT32(petition->payload);
    t_swapFile* file = pidExists(pid);
    if(file == NULL){
        t_packet* response = createPacket(OK_MEM, 0);
        socket_sendPacket(memorySocket, response);
        destroyPacket(response);
        return;
    }
    for(int i = 0; i < file->maxPages; i++){
        if(file->entries[i].pid == pid){
            swapFile_clearAtIndex(file, i);
            file->entries[i].used = false;
        }
    }
}

void sayGoodbye(t_packet* petition, int memorySocket){
    t_packet* response = createPacket(OK_MEM, 0);
    socket_sendPacket(memorySocket, response);
    destroyPacket(response);
    return;
}

void (*swapHandler[MAX_MEM_MSGS])(t_packet* petition, int memorySocket) =
{
    savePage,
    loadPage,
    eraseProcess,
    sayGoodbye
};