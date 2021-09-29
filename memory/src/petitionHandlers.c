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
        void* firstPage = calloc(1, memoryConfig->pageSize);
        t_HeapMetadata* firstAlloc = (t_HeapMetadata*) firstPage;
        firstAlloc->isFree = true;
        firstAlloc->prevAlloc = NULL;
        firstAlloc->nextAlloc = NULL;
        bool ok = createPage(pid, firstPage);
        free(firstPage);
        if(!ok){
            streamAdd_INT32(response->payload, matePtr);
            socket_sendPacket(clientSocket, response);
            destroyPacket(response);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Proceso %u: No pudo asignar %i bytes, no hay lugar para mas procesos", pid, mallocSize);
            pthread_mutex_unlock(&mutex_log);

            return true;
        }
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
            bool rc;
            for(int i = 0; i < newPages; i++){
                void* newPage = calloc(1, memoryConfig->pageSize);
                rc = createPage(pid, newPage);
                free(newPage);
            }
            if(rc == false){
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

bool freeHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memfree (direccion %i)", pid, matePtr);
    pthread_mutex_unlock(&mutex_log);
    
    //TODO: Recorrer todos los allocs bien hasta dar con el que tiene al matePtr
    // No solamente tomar los bytes previos asumiendo que es una direccion correcta.

    int32_t thisAllocPtr = matePtr - sizeof(t_HeapMetadata);
    t_HeapMetadata* thisAlloc = heap_read(pid, thisAllocPtr, sizeof(t_HeapMetadata));
    
    thisAlloc->isFree = true;
    heap_write(pid, thisAllocPtr, sizeof(t_HeapMetadata), thisAlloc);
    free(thisAlloc);

    //TODO: Consolidar el free a derecha, luego izquierda y liberar paginas sobrantes si esta al final.
    
    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: hizo free en la direccion %i", pid, matePtr);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

bool memreadHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    int32_t readSize = streamTake_INT32(petition->payload);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memread (direccion %i, size %i)", pid, matePtr, readSize);
    pthread_mutex_unlock(&mutex_log);

    //TODO: Recorrer todos los allocs revisando si el pedazo a leer corresponde a algo dentro de un alloc valido

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

    //TODO: Recorrer todos los allocs revisando si el pedazo a escribir corresponde a algo dentro de un alloc valido

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memwrite (direccion %i, size %i)", pid, matePtr, writeSize);
    pthread_mutex_unlock(&mutex_log);

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

    swapInterface_eraseProcess(swapInterface, pid);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: se da de baja en memoria, liberando sus paginas de MP", pid);
    pthread_mutex_unlock(&mutex_log);

    for(int i = 0; i < pageTable_countPages(pageTable, pid); i++){
        if(pageTable_isPresent(pageTable, pid, i)){
            int frame = pageTable_getFrame(pageTable, pid, i);
            ram_clearFrameMetadata(ram, frame);
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
    disconnectionHandler
};