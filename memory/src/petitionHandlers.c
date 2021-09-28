#include "memory.h"

/**
 * @DESC: Funcion para ubicar en MP una pagina previamente existente
 */
int32_t getFrame(uint32_t pid, int32_t page){
    int32_t frame = -1;

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Buscando frame de PID:%-10u - Page:%-10i", pid, page);
    pthread_mutex_unlock(&mutex_log);

    // Se busca en la TLB
    frame = TLB_findFrame(tlb, pid, page);
    // Hit! ya retornamos
    if(frame != -1){
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "TLB Hit - PID:%-10u - Page:%-10i - Frame:%-10i", pid, page, frame);
        pthread_mutex_unlock(&mutex_log);
        return frame;
    }

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "TLB Miss - PID:%-10u - Page:%-10i", pid, page);
    pthread_mutex_unlock(&mutex_log);

    // Hubo un miss, seguimos y buscamos en la tabla de paginas
    if(pageTable_isPresent(pageTable, pid, page)){
        // Estaba presente, actualizamos TLB y retornamos frame
        frame = pageTable_getFrame(pageTable, pid, page);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Pagina presente en MP", pid, page);
        pthread_mutex_unlock(&mutex_log);

        TLB_addEntry(tlb, pid, page, frame);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "TLB Actualizada - PID:%-10u - Page:%-10i - Frame:%-10i", pid, page, frame);
        pthread_mutex_unlock(&mutex_log);

        return frame;
    }

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Pagina en Swap", pid, page);
    pthread_mutex_unlock(&mutex_log);

    // No estaba presente en MP, esta en Swap. veamos si hay frames libres en MP
    frame = ram_findFreeFrame(ram, pid);

    // Dio -2 porque estamos en criterio fijo y no hay cabida para otro nuevo proceso
    if(frame == -2) return -2;

    // No hay frames vacios, agarramos una victima :(
    if(frame == -1){
        frame = ram_getVictimIndex(ram, pid);
        

        //Revisamos si el frame tuvo alguna modificacion que amerite actualizarlo en swap
        t_frameMetadata* frameInfo = ram_getFrameMetadata(ram, frame);

        //Se liberan las referencias que enlazan a ese frame con su antigua pagina
        TLB_clearIfExists(tlb, frameInfo->pid, frameInfo->page, frame);
        pageTable_setPresent(pageTable, frameInfo->pid, frameInfo->page, false);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "No hay frames libres, victima elegida: PID:%-10u - Page:%-10i - Frame:%-10i", frameInfo->pid, frameInfo->page, frame);
        pthread_mutex_unlock(&mutex_log);

        if(frameInfo->modified){
            void* frameData = ram_getFrame(ram, frame);
            swapInterface_savePage(swapInterface, frameInfo->pid, frameInfo->page, frameData);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "El frame tenia modificaciones, se lo salvo en swap");
            pthread_mutex_unlock(&mutex_log);
        }
    }
    else{
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Se encontro un Frame libre:%-10i", frame);
        pthread_mutex_unlock(&mutex_log);
    }

    // Ya tenemos un frame listo, buscamos nuestros datos de swap y agarramos ese frame
    ram_replaceFrameMetadata(ram, frame, pid, page);
    void* pageData = swapInterface_loadPage(swapInterface, pid, page);
    ram_replaceFrame(ram, frame, pageData);
    free(pageData);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Pagina recuperada desde swap y cargada al frame");
    pthread_mutex_unlock(&mutex_log);

    //marcamos la pagina como presente en la tabla de paginas y actualizamos TLB
    pageTable_setPresent(pageTable, pid, page, true);
    TLB_addEntry(tlb, pid, page, frame);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "TLB Actualizada - PID:%-10u - Page:%-10i - Frame:%-10i", pid, page, frame);
    pthread_mutex_unlock(&mutex_log);

    return frame;
}

/**
 * @DESC: Lee "size" bytes del heap de un programa. Abstrayendo de la idea de paginas.
 * @param pid: PID del programa
 * @param logicAddress: direccion logica de donde empezar a leer
 * @param size: cantidad de bytes a leer del heap
 * @return void*: puntero malloc'd (con size bytes) con los datos solicitados
 */
void* heap_read(uint32_t pid, int32_t logicAddress, int size){
    void* content = malloc(size);
    int contentOffset = 0;
    int32_t startPage = logicAddress / memoryConfig->pageSize;
    int32_t startOffset = logicAddress % memoryConfig->pageSize;
    int32_t finishPage = (logicAddress + size - 1) / memoryConfig->pageSize;
    int32_t finishOffset = (logicAddress + size - 1) % memoryConfig->pageSize;
    
    int32_t frame = -1;
    void* frameContent = NULL;

    // Si la data esta toda en una sola pagina, hacemos esto una sola vez
    if(startPage == finishPage){
        frame = getFrame(pid, startPage);
        frameContent = ram_getFrame(ram, frame);
        memcpy(content, frameContent + startOffset, size);
        return content;
    }

    //Si la data esta entre varias paginas, las recorremos
    for(int i = startPage; i <= finishPage; i++){
        frame = getFrame(pid, i);
        frameContent = ram_getFrame(ram, frame);

        // Si estamos en la primer pagina de la data, copiamos desde el offset inicial hasta el fin de pagina
        if(i == startPage){ 
            memcpy(content+contentOffset, frameContent+startOffset, memoryConfig->pageSize - startOffset);
            contentOffset += memoryConfig->pageSize - startOffset;
        }

        // Si estamos en la ultima pagina de la data, seguimos copiando desde el inicio de la pagina hasta el offset final
        else if (i == finishPage) {
            memcpy(content+contentOffset, frameContent, finishOffset + 1);
            contentOffset += finishOffset + 1;
        }

        // Si estamos en una pagina que no es la primera o la ultima (una del medio) copiamos toda la pagina
        else{
            memcpy(content+contentOffset, frameContent, memoryConfig->pageSize);
            contentOffset += memoryConfig->pageSize;
        }    
    }

    return content;
}

void heap_write(uint32_t pid, int32_t logicAddress, int size, void* data){
    int32_t startPage = logicAddress / memoryConfig->pageSize;
    int32_t startOffset = logicAddress % memoryConfig->pageSize;
    int32_t finishPage = (logicAddress + size - 1) / memoryConfig->pageSize;
    int32_t finishOffset = (logicAddress + size - 1) % memoryConfig->pageSize;

    int dataOffset = 0;

    int32_t frame = -1;

    // Si la data a reescribir esta toda en una sola pagina, hacemos esto una sola vez
    if(startPage == finishPage){
        frame = getFrame(pid, startPage);
        ram_editFrame(ram, frame, startOffset, data, size);
        return;
    }

    //Si la data debe ser escrita entre varias paginas, las recorremos
    for(int i = startPage; i <= finishPage; i++){
        frame = getFrame(pid, i);

        // Si estamos en la primer pagina de la data, copiamos desde el offset inicial hasta el fin de pagina
        if(i == startPage){ 
            ram_editFrame(ram, frame, startOffset, data + dataOffset, memoryConfig->pageSize - startOffset);
            dataOffset += memoryConfig->pageSize - startOffset;
        }

        // Si estamos en la ultima pagina de la data, seguimos copiando desde el inicio de la pagina hasta el offset final
        else if (i == finishPage) {
            ram_editFrame(ram, frame, 0, data + dataOffset, finishOffset + 1);
            dataOffset += finishOffset + 1;
        }

        // Si estamos en una pagina que no es la primera o la ultima (una del medio) copiamos toda la pagina
        else{
            ram_editFrame(ram, frame, 0, data + dataOffset, memoryConfig->pageSize);
            dataOffset += memoryConfig->pageSize;
        }    
    }
}

bool createPage(uint32_t pid, void* data){
    int32_t frame = ram_findFreeFrame(ram, pid);
    if (frame == -2) {
        
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "No hay lugar para mas procesos por el momento");
        pthread_mutex_unlock(&mutex_log);

        return false;
    }
    int32_t pageNumber = pageTable_countPages(pageTable, pid);
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Nueva pagina creada - PID:%-10u - Page:%-10i", pid, pageNumber);
    pthread_mutex_unlock(&mutex_log);

    if (frame != -1){
        
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Se encontro un frame vacio para la nueva pagina: %-10i", frame);
        pthread_mutex_unlock(&mutex_log);

        ram_replaceFrame(ram, frame, data);
        t_frameMetadata* frameInfo = ram_getFrameMetadata(ram, frame);
        frameInfo->modified = true;
        frameInfo->page = pageNumber;
        frameInfo->pid = pid;
        frameInfo->chance = true;
        frameInfo->isFree = false;
        frameInfo->lastUsed = ++ram->LRU_clock;
        pageTable_addPage(pageTable, pid, frame, true);
        return true;
    }
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Guardando la nueva pagina en Swap");
    pthread_mutex_unlock(&mutex_log);

    bool rc = swapInterface_savePage(swapInterface, pid, pageNumber, data);
    if(rc) pageTable_addPage(pageTable, pid, frame, false);
    else{
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "No hay lugar para esa pagina en swap");
        pthread_mutex_unlock(&mutex_log);
    }
    return rc;
}

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
    int32_t matePtr = 0; //Por ahora solo retorna la direccion 0
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memalloc (%i bytes)", pid, mallocSize);
    pthread_mutex_unlock(&mutex_log);

    //Creo la primer pagina si no tiene niguna
    if(pageTable_countPages(pageTable, pid) == 0){
        void* firstPage = malloc(memoryConfig->pageSize);
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
                void* newPage = malloc(memoryConfig->pageSize);
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
        t_HeapMetadata* newLastAlloc = malloc(sizeof(t_HeapMetadata));
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
    
    int32_t thisAllocPtr = matePtr - sizeof(t_HeapMetadata);
    t_HeapMetadata* thisAlloc = heap_read(pid, thisAllocPtr, sizeof(t_HeapMetadata));
    
    thisAlloc->isFree = true;
    heap_write(pid, thisAllocPtr, sizeof(t_HeapMetadata), thisAlloc);
    free(thisAlloc);

    //TODO: Consolidar el free a izquierda y derecha, y liberar paginas
    
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