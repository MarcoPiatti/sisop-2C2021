#include "swapInterface.h"

t_swapInterface* swapInterface_create(char* swapIp, char* swapPort, int pageSize){
    t_swapInterface* self = malloc(sizeof(t_swapInterface));
    self->socket = connectToServer(swapIp, swapPort);
    pthread_mutex_init(&self->mutex, NULL);
    self->pageSize = pageSize;
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
    if (reply->header == OK_MEM) rc == true;
    else if (reply->header == ERROR_MEM) rc == false;
    destroyPacket(request);
    destroyPacket(reply);
    return rc;
}

void* swapInterface_loadPage(t_swapInterface* self, uint32_t pid, int32_t pageNumber){
    t_packet* request = createPacket(LOAD_PAGE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(request->payload, pid);
    streamAdd_INT32(request->payload, pageNumber);
    
    pthread_mutex_lock(&self->mutex);
    socket_sendPacket(self->socket, request);
    t_packet* reply = socket_getPacket(self->socket);
    pthread_mutex_unlock(&self->mutex);

    void* pageData = NULL;
    if (reply->header == ERROR_MEM) pageData = NULL;
    else if (reply->header == PAGE_DATA){
        streamTake(reply->payload, &pageData, self->pageSize);
    }
    destroyPacket(request);
    destroyPacket(reply);
    return pageData;
}

bool swapInterace_eraseProcess(t_swapInterface* self, uint32_t pid){
    t_packet* request = createPacket(LOAD_PAGE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(request->payload, pid);
    
    pthread_mutex_lock(&self->mutex);
    socket_sendPacket(self->socket, request);
    t_packet* reply = socket_getPacket(self->socket);
    pthread_mutex_unlock(&self->mutex);

    bool rc;
    if (reply->header == OK_MEM) rc = true;
    else rc = false;
    destroyPacket(request);
    destroyPacket(reply);
    return rc;
}
