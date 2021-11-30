#include "swap.h"

void savePage(t_packet *received, int clientSocket){
    uint32_t pid = streamTake_UINT32(received->payload);
    int32_t pageNumber = streamTake_INT32(received->payload);
    void* pageData = NULL;
    streamTake(received->payload, &pageData, (size_t)swapConfig->pageSize);

    bool rc = asignacion(pid, pageNumber, pageData);
    
    t_packet* response;
    if(rc) response = createPacket(SWAP_OK, 0);
    else response = createPacket(SWAP_ERROR, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    free(pageData);
}

void readPage(t_packet *received, int clientSocket){
    uint32_t pid = streamTake_UINT32(received->payload);
    int32_t pageNumber = streamTake_INT32(received->payload);
    t_swapFile* file = pidExists(pid);
    if(file == NULL){
        t_packet* response = createPacket(SWAP_ERROR, 0);
        socket_sendPacket(clientSocket, response);
        destroyPacket(response);
        return;
    }
    int index = swapFile_getIndex(file, pid, pageNumber);
    if(index == -1){
        t_packet* response = createPacket(SWAP_ERROR, 0);
        socket_sendPacket(clientSocket, response);
        destroyPacket(response);
        return;
    }
    void* pageData = swapFile_readAtIndex(file, index);
    t_packet* response = createPacket(PAGE, (size_t)swapConfig->pageSize);
    streamAdd(response->payload, pageData, swapConfig->pageSize);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    free(pageData);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Archivo %s: se recupero la pagina %i del proceso %i en el indice %i", file->path, pageNumber, pid, index);
    pthread_mutex_unlock(&mutex_log);
}

void destroyPage(t_packet *received, int clientSocket){
    uint32_t pid = streamTake_UINT32(received->payload);
    uint32_t page = streamTake_INT32(received->payload);
    t_swapFile* file = pidExists(pid);

    if(file == NULL){
        t_packet* response = createPacket(SWAP_ERROR, 0);
        socket_sendPacket(clientSocket, response);
        destroyPacket(response);
        return;
    }
    int index = swapFile_getIndex(file, pid, page);
    if(index == -1){
        t_packet* response = createPacket(SWAP_ERROR, 0);
        socket_sendPacket(clientSocket, response);
        destroyPacket(response);
        return;
    }
    swapFile_clearAtIndex(file, index);
    file->entries[index].used = false;

    t_packet* response = createPacket(SWAP_OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Archivo %s: se elimino la pagina %i del proceso %u", file->path, page, pid);
    pthread_mutex_unlock(&mutex_log);

    return;
}

void memDisconnect(t_packet *received, int clientSocket){
    t_packet* response = createPacket(SWAP_OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
}

void (*petitionHandler[MAX_MEM_PETITIONS])(t_packet *received, int clientSocket) = {
    savePage,
    readPage,
    destroyPage,
    memDisconnect
};
