#include "memory.h"

bool initHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    
    pageTable_addProcess(pageTable, pid);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: se da de alta en memoria", pid);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

bool mallocHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t mallocSize = streamTake_INT32(petition->payload);
    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);
    int32_t matePtr = 0;
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memalloc (%i bytes)", pid, mallocSize);
    pthread_mutex_unlock(&mutex_log);

    //Creo la primer pagina si no tiene niguna
    if(pageTable_countPages(pageTable, pid) == 0){
        int32_t rc = createPage(pid);
        if(rc == -1){
            streamAdd_INT32(response->payload, matePtr);
            socket_sendPacket(clientSocket, response);
            destroyPacket(response);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Proceso %u: No pudo asignar %i bytes, no hay lugar para mas procesos", pid, mallocSize);
            pthread_mutex_unlock(&mutex_log);

            return true;
        }
        t_HeapMetadata* firstAlloc = malloc(sizeof(t_HeapMetadata));
        firstAlloc->isFree = true;
        firstAlloc->prevAlloc = NULL;
        firstAlloc->nextAlloc = NULL;
        heap_write(pid, 0, sizeof(t_HeapMetadata), firstAlloc);
        free(firstAlloc);
    }

    t_HeapMetadata* thisMalloc = NULL;
    int32_t thisMallocAddr = 0;
    int32_t thisMallocSize = 0;

    thisMalloc = heap_read(pid, 0, sizeof(t_HeapMetadata));
    //Primero recorro todos los mallocs internos, revisando si hay alguno reusable
    while(thisMalloc->nextAlloc != NULL){
        thisMallocSize = thisMalloc->nextAlloc - thisMallocAddr - sizeof(t_HeapMetadata);
        if(thisMalloc->isFree){ 
            //Si el espacio del alloc es muy justito como para no poder crear otro con lo sobrante, debe tener justo mismo espacio
            if(thisMallocSize <= sizeof(t_HeapMetadata) + 1 && thisMallocSize == mallocSize){
                thisMalloc->isFree = false;
                heap_write(pid, thisMallocAddr, sizeof(t_HeapMetadata), thisMalloc);
                free(thisMalloc);
                matePtr = thisMallocAddr + sizeof(t_HeapMetadata);
                break;
            }
            //Si tiene espacio de sobra como para meter un alloc free de al menos 1B en el medio...
            else if(thisMallocSize > sizeof(t_HeapMetadata) + 1 && thisMallocSize - sizeof(t_HeapMetadata) - 1 >= mallocSize){
                thisMalloc->isFree = false;
                matePtr = thisMallocAddr + sizeof(t_HeapMetadata);
                t_HeapMetadata* newMiddleAlloc = malloc(sizeof(t_HeapMetadata));
                newMiddleAlloc->isFree = true;
                newMiddleAlloc->nextAlloc = thisMalloc->nextAlloc;
                newMiddleAlloc->prevAlloc = thisMallocAddr;
                thisMalloc->nextAlloc = matePtr + mallocSize;
                heap_write(pid, thisMallocAddr, sizeof(t_HeapMetadata), thisMalloc);
                heap_write(pid, matePtr + mallocSize, sizeof(t_HeapMetadata), newMiddleAlloc);
                free(newMiddleAlloc);
                free(thisMalloc);
                break;
            }
        }
        thisMallocAddr = thisMalloc->nextAlloc;
        free(thisMalloc);
        thisMalloc = heap_read(pid, thisMallocAddr, sizeof(t_HeapMetadata));
    }

    //No habia ningun alloc ya existente que nos haya servido, trabajamos con el ultimo alloc
    if(matePtr == 0){
        int32_t thisMallocOffset = thisMallocAddr % memoryConfig->pageSize;
        thisMallocSize = memoryConfig->pageSize - thisMallocOffset - sizeof(t_HeapMetadata);

        //Si no alcanza el lugar para crear otro malloc en la misma pagina, creamos mas paginas
        if(thisMallocSize <= sizeof(t_HeapMetadata) + 1 || thisMallocSize - sizeof(t_HeapMetadata) - 1 < mallocSize){
            int newPages = 1 + (mallocSize - thisMallocSize + sizeof(t_HeapMetadata) + 1) / memoryConfig->pageSize;
            //Crea todas las paginas necesarias, y si alguna falla, borra todas las anteriores retroactivamente
            int32_t firstPage = createPage(pid);
            int32_t lastPage = 0;
            if(firstPage != -1){
                for(int i = 1; i < newPages; i++){
                    lastPage = createPage(pid);
                    if(lastPage == -1){
                        for(int j = lastPage-1; j >= firstPage; j--){
                            swapInterface_erasePage(swapInterface, pid, j);
                            pageTable_removePage(pageTable, pid, j);
                        }
                        break;
                    }
                }
            }
            if(firstPage == -1 || lastPage == -1){
                streamAdd_INT32(response->payload, matePtr);
                socket_sendPacket(clientSocket, response);
                destroyPacket(response);

                pthread_mutex_lock(&mutex_log);
                log_info(logger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", pid, mallocSize, matePtr);
                pthread_mutex_unlock(&mutex_log);

                free(thisMalloc);
                return true;
            }
        }
        matePtr = thisMallocAddr + sizeof(t_HeapMetadata);
        thisMalloc->isFree = false;
        t_HeapMetadata* newLastAlloc = calloc(1, sizeof(t_HeapMetadata));
        newLastAlloc->isFree = true;
        newLastAlloc->nextAlloc = thisMalloc->nextAlloc;
        newLastAlloc->prevAlloc = thisMallocAddr;
        thisMalloc->nextAlloc = matePtr + mallocSize;
        heap_write(pid, matePtr + mallocSize, sizeof(t_HeapMetadata), newLastAlloc);
        heap_write(pid, thisMallocAddr, sizeof(t_HeapMetadata), thisMalloc);
        free(newLastAlloc);
        free(thisMalloc);
    }
    streamAdd_INT32(response->payload, matePtr);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: Se asigno %i bytes en la direccion %i", pid, mallocSize, matePtr);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

bool validMallocAddress(uint32_t pid, int32_t matePtr){
    if (matePtr / memoryConfig->pageSize >= pageTable_countPages(pageTable, pid)) return false;
    t_HeapMetadata* thisMalloc = NULL;
    int32_t thisMallocAddr = 0;
    bool correct = false;
    thisMalloc = heap_read(pid, 0, sizeof(t_HeapMetadata));
    while(thisMalloc->nextAlloc != NULL){
        if(!thisMalloc->isFree){
            if(matePtr == thisMallocAddr + sizeof(t_HeapMetadata)){
                correct = true;
                break;
            }
        }
        thisMallocAddr = thisMalloc->nextAlloc;
        free(thisMalloc);
        thisMalloc = heap_read(pid, thisMallocAddr, sizeof(t_HeapMetadata));
    }
    free(thisMalloc);
    return correct;
}

bool freeHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memfree (direccion %i)", pid, matePtr);
    pthread_mutex_unlock(&mutex_log);
    
    //Si la direccion es invalida corta y retorna error al carpincho
    if(!validMallocAddress(pid, matePtr)){
        t_packet* response = createPacket(ERROR, 0);
        socket_sendPacket(clientSocket, response);
        free(response);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: trato de hacer free de una direccion invalida", pid);
        pthread_mutex_unlock(&mutex_log);

        return true;
    }

    int32_t thisAllocPtr, rightAllocPtr, leftAllocPtr, finalAllocPtr;
    t_HeapMetadata *thisAlloc, *rightAlloc, *leftAlloc, *finalAlloc;
    
    thisAllocPtr = matePtr - sizeof(t_HeapMetadata);
    thisAlloc = heap_read(pid, thisAllocPtr, sizeof(t_HeapMetadata));
    finalAllocPtr = thisAllocPtr;
    finalAlloc = thisAlloc;

    //Nuestro alloc absorbe al derecho si el derecho esta libre
    if(thisAlloc->nextAlloc != NULL){
        rightAllocPtr = thisAlloc->nextAlloc;
        rightAlloc = heap_read(pid, rightAllocPtr, sizeof(t_HeapMetadata));
        if(rightAlloc->isFree){
            thisAlloc->nextAlloc = rightAlloc->nextAlloc;
        }
    }

    //El alloc izquierdo absorbe a nuestro alloc si el izquierdo esta libre
    leftAllocPtr = thisAlloc->prevAlloc;
    leftAlloc = heap_read(pid, leftAllocPtr, sizeof(t_HeapMetadata));
    if(leftAlloc->isFree){
        leftAlloc->nextAlloc = thisAlloc->nextAlloc;
        finalAlloc = leftAlloc;
        finalAllocPtr = leftAllocPtr;
        heap_write(pid, leftAllocPtr, sizeof(t_HeapMetadata), leftAlloc);
    }

    thisAlloc->isFree = true;
    heap_write(pid, thisAllocPtr, sizeof(t_HeapMetadata), thisAlloc);

    //Se liberan paginas sobrantes si esta al final.
    if(finalAlloc->nextAlloc == NULL){

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: Se borran paginas libres en el free", pid, matePtr);
        pthread_mutex_unlock(&mutex_log);

        int32_t firstFreeByte = finalAllocPtr + sizeof(t_HeapMetadata);
        int32_t pageToDelete = 1 + firstFreeByte / memoryConfig->pageSize;
        int totalPages = pageTable_countPages(pageTable, pid);
        for(int i = totalPages-1; i >= pageToDelete; i--){
            if(pageTable_isPresent(pageTable, pid, i)){
                int frame = pageTable_getFrame(pageTable, pid, i);
                ram_clearFrameMetadata(ram, frame);
                TLB_clearIfExists(tlb, pid, i, frame);
            }
            pageTable_removePage(pageTable, pid, i);
            swapInterface_erasePage(swapInterface, pid, i);
        }
    }
    
    free(thisAlloc);
    free(leftAlloc);
    free(rightAlloc);
    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: hizo free en la direccion %i", pid, matePtr);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

bool validAddressRange(uint32_t pid, int32_t matePtr, int32_t readSize){
    t_HeapMetadata* thisMalloc = NULL;
    int32_t thisMallocAddr = 0;
    int32_t thisMallocSize = 0;
    bool correct = false;
    thisMalloc = heap_read(pid, 0, sizeof(t_HeapMetadata));
    while(thisMalloc->nextAlloc != NULL){
        thisMallocSize = thisMalloc->nextAlloc - thisMallocAddr - sizeof(t_HeapMetadata);
        if(!thisMalloc->isFree){
            if(matePtr >= thisMallocAddr + sizeof(t_HeapMetadata) && matePtr + readSize <= thisMallocAddr + sizeof(t_HeapMetadata) + thisMallocSize){
                correct = true;
                break;
            }
        }
        thisMallocAddr = thisMalloc->nextAlloc;
        free(thisMalloc);
        thisMalloc = heap_read(pid, thisMallocAddr, sizeof(t_HeapMetadata));
    }
    free(thisMalloc);
    return correct;
}

bool memreadHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    int32_t readSize = streamTake_INT32(petition->payload);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memread (direccion %i, size %i)", pid, matePtr, readSize);
    pthread_mutex_unlock(&mutex_log);

    if(!validAddressRange(pid, matePtr, readSize)){
        t_packet* response = createPacket(ERROR, 0);
        socket_sendPacket(clientSocket, response);
        free(response);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: trato de acceder a un address space invalido", pid);
        pthread_mutex_unlock(&mutex_log);

        return true;
    }

    void* contents = heap_read(pid, matePtr, readSize);
    t_packet* response = createPacket(MEM_CHUNK, INITIAL_STREAM_SIZE);
    streamAdd_INT32(response->payload, readSize);
    streamAdd(response->payload, contents, readSize);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: leyo %i bytes de la direccion %i", pid, readSize, matePtr);
    pthread_mutex_unlock(&mutex_log);
    free(contents);
    return true;
}

bool memwriteHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    int32_t writeSize = streamTake_INT32(petition->payload);
    void* writeData = NULL; 
    streamTake(petition->payload, &writeData, writeSize);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memwrite (direccion %i, size %i)", pid, matePtr, writeSize);
    pthread_mutex_unlock(&mutex_log);

    if(!validAddressRange(pid, matePtr, writeSize)){
        t_packet* response = createPacket(ERROR, 0);
        socket_sendPacket(clientSocket, response);
        free(response);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: trato de acceder a un address space invalido", pid);
        pthread_mutex_unlock(&mutex_log);

        return true;
    }

    heap_write(pid, matePtr, writeSize, writeData);

    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: escribio %i bytes en la direccion %i", pid, writeSize, matePtr);
    pthread_mutex_unlock(&mutex_log);

    free(writeData);
    return true;
}

bool terminationHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: se da de baja en memoria, liberando sus paginas de Swap", pid);
    pthread_mutex_unlock(&mutex_log);

    int32_t maxPages = pageTable_countPages(pageTable, pid);
    for(int i = 0; i < maxPages; i++){
        swapInterface_erasePage(swapInterface, pid, i);
    }

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: se da de baja en memoria, liberando sus paginas de MP", pid);
    pthread_mutex_unlock(&mutex_log);

    for(int i = 0; i < maxPages; i++){
        if(pageTable_isPresent(pageTable, pid, i)){
            int frame = pageTable_getFrame(pageTable, pid, i);
            ram_clearFrameMetadata(ram, frame);
            TLB_clearIfExists(tlb, pid, i, frame);
        }
    }

    pageTable_removeProcess(pageTable, pid);

    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: se da de baja en memoria", pid);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

bool disconnectionHandler(int clientSocket, t_packet* petition){
    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    close(clientSocket);
    destroyPacket(response);
    return false;
}

bool suspensionHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t maxPages = pageTable_countPages(pageTable, pid);
    int32_t frame;
    t_frameMetadata* frameInfo;
    for(int i = 0; i < maxPages; i++){
        if(pageTable_isPresent(pageTable, pid, i)){
            frame = pageTable_getFrame(pageTable, pid, i);
            frameInfo = ram_getFrameMetadata(ram, frame);
            if(frameInfo->modified){
                void* frameData = ram_getFrame(ram, frame);
                swapInterface_savePage(swapInterface, pid, i, frameData);
                frameInfo->modified = false;
            }
            if(ram->assignmentType == fixed){
                frameInfo->isFree = true;
            }
        }
    }
    return true;
}

bool (*petitionHandlers[MAX_PETITIONS])(int clientSocket, t_packet* petition) =
{
    initHandler,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mallocHandler,
    freeHandler,
    memreadHandler,
    memwriteHandler,
    terminationHandler,
    disconnectionHandler,
    suspensionHandler
};