#include "swapInterface.h"
#include <unistd.h>

t_swapInterface* swapInterface_create(char* swapIp, char* swapPort, int pageSize, swapHeader algorithm){
    t_swapInterface* self = malloc(sizeof(t_swapInterface));
    self->socket = connectToServer(swapIp, swapPort);
    pthread_mutex_init(&self->mutex, NULL);
    self->pageSize = pageSize;
    socket_sendHeader(self->socket, (uint8_t) algorithm);
    return self;
}

void swapInterface_destroy(t_swapInterface* self){
    close(self->socket);
    pthread_mutex_destroy(&self->mutex);
    free(self);
}

bool swapInterface_savePage(t_swapInterface* self, uint32_t pid, int32_t pageNumber, void* pageContent){
    t_packet* request = createPacket(SAVE_PAGE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(request->payload, pid);
    streamAdd_INT32(request->payload, pageNumber);
    streamAdd(request->payload, pageContent, self->pageSize);
    
    pthread_mutex_lock(&self->mutex);
    socket_sendPacket(self->socket, request);
    t_packet* reply = socket_getPacket(self->socket);
    pthread_mutex_unlock(&self->mutex);

    bool rc;
    if (reply->header == SWAP_OK) rc = true;
    else if (reply->header == SWAP_ERROR) rc = false;
    destroyPacket(request);
    destroyPacket(reply);
    return rc;
}

void* swapInterface_loadPage(t_swapInterface* self, uint32_t pid, int32_t pageNumber){
    t_packet* request = createPacket(READ_PAGE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(request->payload, pid);
    streamAdd_INT32(request->payload, pageNumber);
    
    pthread_mutex_lock(&self->mutex);
    socket_sendPacket(self->socket, request);
    t_packet* reply = socket_getPacket(self->socket);
    pthread_mutex_unlock(&self->mutex);

    void* pageData = NULL;
    if (reply->header == SWAP_ERROR) pageData = NULL;
    else if (reply->header == PAGE){
        streamTake(reply->payload, &pageData, self->pageSize);
    }
    destroyPacket(request);
    destroyPacket(reply);
    return pageData;
}

bool swapInterface_erasePage(t_swapInterface* self, uint32_t pid, int32_t page){
    t_packet* request = createPacket(DESTROY_PAGE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(request->payload, pid);
    streamAdd_INT32(request->payload, page);

    pthread_mutex_lock(&self->mutex);
    socket_sendPacket(self->socket, request);
    t_packet* reply = socket_getPacket(self->socket);
    pthread_mutex_unlock(&self->mutex);

    bool rc;
    if (reply->header == SWAP_OK) rc = true;
    else rc = false;
    destroyPacket(request);
    destroyPacket(reply);
    return rc;
}
