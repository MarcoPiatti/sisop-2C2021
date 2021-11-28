#include "swap.h"

void savePage(t_packet *received, int clientSocket){
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
    free(pageData);
}

void readPage(t_packet *received, int clientSocket){
    ;
}

void destroyPage(t_packet *received, int clientSocket){
    ;
}

void memDisconnect(t_packet *received, int clientSocket){
    t_packet* response = createPacket(OK_MEM, 0);
    socket_sendPacket(memorySocket, response);
    destroyPacket(response);
}

void (*petitionHandler[MAX_MEM_PETITIONS])(t_packet *received, int clientSocket) = {
    savePage,
    readPage,
    destroyPage,
    memDisconnect
};
