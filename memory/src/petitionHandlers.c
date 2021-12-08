#include <commons/string.h>
#include "memory.h"
#include "stdint.h"
#include "swapInterface.h"
#include "utils.h"
#include <commons/memory.h>


//  AUX
int32_t createPage(uint32_t PID){
    pthread_mutex_lock(&pageTablesMut);
        t_pageTable* table = getPageTable(PID, pageTables);
    pthread_mutex_unlock(&pageTablesMut);
    
    int32_t pageNumber = pageTableAddEntry(table, 0);
    void* newPageContents = calloc(1, config->pageSize);
    if(swapInterface_savePage(swapInterface, PID, pageNumber, newPageContents)){
        return pageNumber;
    }
    return -1;
}

void deleteLastPages(uint32_t PID, uint32_t lastAllocAddr){
    
    uint32_t firstToDelete = (lastAllocAddr + 9) / config->pageSize;
    
    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t lastToDelete = pt->pageQuantity - 1;
    pthread_mutex_unlock(&pageTablesMut);
    
    for (uint32_t i = lastToDelete; i > firstToDelete; i--){
        swapInterface_erasePage(swapInterface, PID, i);
        
        pageTable_destroyLastEntry(pt);
    }
}

/**
 * @DESC: Lee "size" bytes del heap de un programa. Abstrayendo de la idea de paginas.
 * @param PID: PID del programa
 * @param logicAddress: direccion logica de donde empezar a leer
 * @param size: cantidad de bytes a leer del heap
 * @return void*: puntero malloc'd (con size bytes) con los datos solicitados
 */
void* heap_read(uint32_t PID, int32_t logicAddress, int size){
    void* content = malloc(size);
    int contentOffset = 0;
    int32_t startPage = logicAddress / config->pageSize;
    int32_t startOffset = logicAddress % config->pageSize;
    int32_t finishPage = (logicAddress + size - 1) / config->pageSize;
    int32_t finishOffset = (logicAddress + size - 1) % config->pageSize;
    
    int32_t frame = -1;
    void* frameContent = NULL;

    // Si la data esta toda en una sola pagina, hacemos esto una sola vez
    if(startPage == finishPage){
        frame = getFrame(PID, startPage);
        frameContent = ram_getFrame(ram, frame);
        memcpy(content, frameContent + startOffset, size);
        return content;
    }

    //Si la data esta entre varias paginas, las recorremos
    for(int i = startPage; i <= finishPage; i++){
        frame = getFrame(PID, i);
        frameContent = ram_getFrame(ram, frame);

        // Si estamos en la primer pagina de la data, copiamos desde el offset inicial hasta el fin de pagina
        if(i == startPage){ 
            memcpy(content+contentOffset, frameContent+startOffset, config->pageSize - startOffset);
            contentOffset += config->pageSize - startOffset;
        }

        // Si estamos en la ultima pagina de la data, seguimos copiando desde el inicio de la pagina hasta el offset final
        else if (i == finishPage) {
            memcpy(content+contentOffset, frameContent, finishOffset + 1);
            contentOffset += finishOffset + 1;
        }

        // Si estamos en una pagina que no es la primera o la ultima (una del medio) copiamos toda la pagina
        else{
            memcpy(content+contentOffset, frameContent, config->pageSize);
            contentOffset += config->pageSize;
        }    
    }

    return content;
}

void heap_write(uint32_t PID, int32_t logicAddress, int size, void* data){
    int32_t startPage = logicAddress / config->pageSize;
    int32_t startOffset = logicAddress % config->pageSize;
    int32_t finishPage = (logicAddress + size - 1) / config->pageSize;
    int32_t finishOffset = (logicAddress + size - 1) % config->pageSize;

    int dataOffset = 0;

    int32_t frame = -1;

    // Si la data a reescribir esta toda en una sola pagina, hacemos esto una sola vez
    if(startPage == finishPage){
        frame = getFrame(PID, startPage);
        ram_editFrame(ram, startOffset, frame, data, size);
        return;     
    }

    //Si la data debe ser escrita entre varias paginas, las recorremos
    for(int i = startPage; i <= finishPage; i++){
        frame = getFrame(PID, i);

        // Si estamos en la primer pagina de la data, copiamos desde el offset inicial hasta el fin de pagina
        if(i == startPage){ 
            ram_editFrame(ram, startOffset, frame, data + dataOffset, config->pageSize - startOffset);
            dataOffset += config->pageSize - startOffset;
        }

        // Si estamos en la ultima pagina de la data, seguimos copiando desde el inicio de la pagina hasta el offset final
        else if (i == finishPage) {
            ram_editFrame(ram, 0, frame, data + dataOffset, finishOffset + 1);
            dataOffset += finishOffset + 1;
        }

        // Si estamos en una pagina que no es la primera o la ultima (una del medio) copiamos toda la pagina
        else{
            ram_editFrame(ram, 0, frame, data + dataOffset, config->pageSize);
            dataOffset += config->pageSize;
        }    
    }
}

uint32_t getLastAllocAddr(uint32_t PID) {
    uint32_t metadataAddr = 0;

    while(1){
        t_heapMetadata *currentMetadata = heap_read(PID, metadataAddr, 9);

        if (currentMetadata->nextAlloc == 0){
            free(currentMetadata);
            return metadataAddr;
        }

        metadataAddr = currentMetadata->nextAlloc;
        free(currentMetadata);
    }
}

int32_t getFreeAlloc(uint32_t PID, uint32_t size) {
    uint32_t metadataAddr = 0;

    while(1){
        t_heapMetadata *currentMetadata = heap_read(PID, metadataAddr, 9);

        if (currentMetadata->nextAlloc == 0){
            free(currentMetadata);
            return -1;
        }
        
        uint32_t spaceToNextAlloc = currentMetadata->nextAlloc - metadataAddr - 9;

        if (currentMetadata->isFree && spaceToNextAlloc >= size){
            free(currentMetadata);
            return metadataAddr;
        }

        metadataAddr = currentMetadata->nextAlloc;
        free(currentMetadata);
    }
}

//  HANDLERS.
bool memallocHandler(t_packet* petition, int socket){
    pthread_mutex_lock(&logMut);
        log_debug(memLogger, "Paquete MEMALLOC recibido");
    pthread_mutex_unlock(&logMut);

    uint32_t PID  = streamTake_UINT32(petition->payload);
    uint32_t size = streamTake_INT32(petition->payload);

    pthread_mutex_lock(&logMut);
        log_debug(memLogger, "Haciendo alloc de %u, bytes para carpincho de PID %u.", size, PID);
    pthread_mutex_unlock(&logMut);

    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);

    // Caso carpincho no tienen nada en memoria:
    if (pageTable_isEmpty(PID)) {
        if(createPage(PID) == -1) {
            streamAdd_INT32(response->payload, 0);
            socket_sendPacket(socket, response);
            destroyPacket(response);

            pthread_mutex_lock(&logMut);
                log_debug(memLogger, "No se pudo crear pagina para el carpincho de PID %u.", PID);
            pthread_mutex_unlock(&logMut);
        }
        t_heapMetadata first;
        first.isFree = true;
        first.nextAlloc = 0;
        first.prevAlloc = 0;

        // Se guarda el nuevo alloc en memoria.
        heap_write(PID, 0, 9, &first);
    }


    bool useLastAlloc = false;
    uint32_t currentAllocAddr = 0;

    while (! useLastAlloc){
        t_heapMetadata *currentAlloc = heap_read(PID, currentAllocAddr, 9);
        if (currentAlloc->nextAlloc == 0) {
            free(currentAlloc);
            useLastAlloc = true;
            break;
        } else if (currentAlloc->isFree && currentAlloc->nextAlloc - currentAllocAddr - 9 >= size) {
            free(currentAlloc);
            break;
        }
        currentAllocAddr = currentAlloc->nextAlloc;
        free(currentAlloc);
    }

    // Caso hay alloc libre:
    if (!useLastAlloc){
        t_heapMetadata *found = heap_read(PID, currentAllocAddr, 9);
        uint32_t distanceToNextAlloc = found->nextAlloc - currentAllocAddr - 9;
        streamAdd_INT32(response->payload, currentAllocAddr + 9);
        
        // Si solo hay lugar para la memoria pedida, no se crea un heapMetadata intermedio.
        if (distanceToNextAlloc <= size + 9){
            // Actualizar heapMetadata en ram.
            found->isFree = false;
            heap_write(PID, currentAllocAddr, 9, found);
            free(found);

            socket_sendPacket(socket, response);
            destroyPacket(response);

            return true;
        }

        // Si hay mas lugar, se crea un heapMetadata intermedio para evitar overallocar.
        t_heapMetadata *oldNextAlloc = heap_read(PID, found->nextAlloc, 9);

        // Se crea nuevo alloc entre medio del encontrado y el siguiente.
        t_heapMetadata midAlloc = { .nextAlloc = found->nextAlloc, .prevAlloc = currentAllocAddr, .isFree = true };

        // Se actualizan los allocs que existian previamente.
        found->nextAlloc = currentAllocAddr + 9 + size;
        found->isFree = false;
        oldNextAlloc->prevAlloc = found->nextAlloc;

        // Se actualiza la ram.
        heap_write(PID, currentAllocAddr, 9, found);
        heap_write(PID, found->nextAlloc, 9, &midAlloc);
        heap_write(PID, midAlloc.nextAlloc, 9, oldNextAlloc);

        free(found);
        free(oldNextAlloc);

        socket_sendPacket(socket, response);
        destroyPacket(response);
        return true;
    }
    
    // Caso no hay alloc libre:
    uint32_t lastAllocOffset = currentAllocAddr % config->pageSize;
    uint32_t freeSpaceInPage = config->pageSize - lastAllocOffset - 9;
    uint32_t necessarySize = size + 9;

    t_heapMetadata *lastAlloc = heap_read(PID, currentAllocAddr, 9);

    // Si hay espacio suficiente en la ult pagina, no se crea ninguna nueva.
    if(freeSpaceInPage >= necessarySize) {
        lastAlloc->isFree = false;
        lastAlloc->nextAlloc = currentAllocAddr + 9 + size;

        t_heapMetadata newLastAlloc = { .nextAlloc = 0, .prevAlloc = currentAllocAddr, .isFree = true };

        heap_write(PID, currentAllocAddr, 9, lastAlloc);
        heap_write(PID, lastAlloc->nextAlloc, 9, &newLastAlloc);

        free(lastAlloc);

        streamAdd_INT32(response->payload, currentAllocAddr + 9);
        socket_sendPacket(socket, response);

        destroyPacket(response);
        return true;
    }

    // Si no hay espacio disponible en la ult pagina, se deben crear las necesarias.
    int32_t newPageQty = div_roundUp(necessarySize - freeSpaceInPage, config->pageSize);
    for (int i = 0; i < newPageQty; i++){
        if(createPage(PID) == -1) {
            streamAdd_INT32(response->payload, 0);
            socket_sendPacket(socket, response);
            destroyPacket(response);

            pthread_mutex_lock(&logMut);
                log_info(memLogger, "No se pudo crear pagina para el carpincho de PID %u.", PID);
            pthread_mutex_unlock(&logMut);

            return true;
        }
    }
    // Agregar siguiente y marcar como ocupado al anterior ultimo alloc.
    lastAlloc->isFree = false;
    lastAlloc->nextAlloc = currentAllocAddr + 9 + size;

    // Crear nuevo ultimo alloc.
    t_heapMetadata newLastAlloc = { .nextAlloc = 0, .prevAlloc = currentAllocAddr, .isFree = true };

    // Actualizar ram.
    heap_write(PID, currentAllocAddr, 9, lastAlloc);
    heap_write(PID, lastAlloc->nextAlloc, 9, &newLastAlloc);

    free(lastAlloc);

    streamAdd_INT32(response->payload, currentAllocAddr + 9);
    socket_sendPacket(socket, response);

    return true;    

}

bool memfreeHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);
    int32_t addr = streamTake_INT32(petition->payload);
    t_packet *response;

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t maxAddr = pt->pageQuantity * config->pageSize - 1;
    pthread_mutex_unlock(&pageTablesMut);

    if (addr > maxAddr){
        response = createPacket(ERROR, INITIAL_STREAM_SIZE);
        socket_sendPacket(socket, response);
        destroyPacket(response);
        return true;
    }

    t_heapMetadata *alloc = heap_read(PID, addr - 9, 9);
    uint32_t prevAllocAddr = alloc->prevAlloc;
    uint32_t nextAllocAddr = alloc->nextAlloc;
    uint32_t lastLastAllocAddr = 0;

    t_heapMetadata *nextAlloc;
    t_heapMetadata *prevAlloc;

    bool clearLastPages = false;

    if (nextAllocAddr) {
        nextAlloc = heap_read(PID, nextAllocAddr, 9);

        if (nextAlloc->isFree){
            uint32_t lastAllocAddr = nextAlloc->nextAlloc;

            if (lastAllocAddr){
                t_heapMetadata *lastAlloc = heap_read(PID, lastAllocAddr, 9);
                lastAlloc->prevAlloc = addr - 9;
                heap_write(PID, lastAllocAddr, 9, (void*) lastAlloc);
                free(lastAlloc);
            } else {
                clearLastPages = true;
                lastLastAllocAddr = addr - 9; 
            }

            alloc->nextAlloc = nextAlloc->nextAlloc;
            alloc->isFree = true;
        }

        free(nextAlloc);
    }

    if (prevAllocAddr) {
        prevAlloc = heap_read(PID, prevAllocAddr, 9);

        if (prevAlloc->isFree){
            prevAlloc->nextAlloc = alloc->nextAlloc;

            if (alloc->nextAlloc){
                t_heapMetadata *lastAlloc = heap_read(PID, alloc->nextAlloc, 9);
                lastAlloc->prevAlloc = prevAllocAddr;
                heap_write(PID, alloc->nextAlloc, 9, (void*) lastAlloc);
                free(lastAlloc);
            } else {
                clearLastPages = true;
                lastLastAllocAddr = prevAllocAddr;
            }
        }

        free(prevAlloc);
    }

    if (clearLastPages) {
        deleteLastPages(PID, lastLastAllocAddr);
    }

    response = createPacket(OK, INITIAL_STREAM_SIZE);
    socket_sendPacket(socket, response);
    destroyPacket(response);

    free(alloc);

    return true;
}

bool memreadHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);
    int32_t addr = streamTake_INT32(petition->payload);
    int32_t size = streamTake_INT32(petition->payload);    
    
    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t maxAddr = pt->pageQuantity * config->pageSize - 1;
    pthread_mutex_unlock(&pageTablesMut);

    // Si se intenta leer por fuera del espacio de direcciones del carpincho, devuelve error.
    if (addr + size > maxAddr) {
        t_packet *response = createPacket(ERROR, INITIAL_STREAM_SIZE);
        socket_sendPacket(socket, response);
        destroyPacket(response);
        return true;
    }

    t_packet *response = createPacket(MEM_CHUNK, INITIAL_STREAM_SIZE + size);
    void *data = heap_read(PID, addr, size);
    printf("Leido: %s \n", (char*) data);
    mem_hexdump(ram_getFrame(ram, 0), config->pageSize * 10);
    streamAdd_INT32(response->payload, size);
    streamAdd(response->payload, data, size);
    socket_sendPacket(socket, response);
    destroyPacket(response);
    free(data);
    return true;
}

bool memwriteHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);
    int32_t addr = streamTake_INT32(petition->payload);
    int32_t dataSize = streamTake_INT32(petition->payload);
    void *data = NULL;
    streamTake(petition->payload, &data, dataSize);

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t maxAddr = pt->pageQuantity * config->pageSize - 1;
    pthread_mutex_unlock(&pageTablesMut);

    // Si se intenta escribir por fuera del espacio de direcciones del carpincho, devuelve error.
    if (addr + dataSize > maxAddr) {
        t_packet *response = createPacket(ERROR, INITIAL_STREAM_SIZE);
        socket_sendPacket(socket, response);
        destroyPacket(response);
        return true;
    }

    heap_write(PID, addr, dataSize, data);
    t_packet *response = createPacket(OK, INITIAL_STREAM_SIZE);
    socket_sendPacket(socket, response);
    destroyPacket(response);
    free(data);
    return true;
}

bool suspendHandler(t_packet *petition, int socket) {
    uint32_t PID = streamTake_UINT32(petition->payload);

    pthread_mutex_lock(&pageTablesMut);                     // TODO: Revisar posibilidad de deadlock, verificar logica.
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t pages = pt->pageQuantity;
        for (uint32_t i = 0; i < pages; i++){
            if (pt->entries[i].present){
                void *pageContent = ram_getFrame(ram, pt->entries[i].frame);
                swapInterface_savePage(swapInterface, PID, i, pageContent);
                pthread_mutex_lock(&metadataMut);
                    metadata->entries[pt->entries[i].frame].isFree = true;
                pthread_mutex_unlock(&metadataMut);
                pt->entries[i].present = false;
            }
        }
    pthread_mutex_unlock(&pageTablesMut); 

    pthread_mutex_lock(&metadataMut);
        for (uint32_t i = 0; i < config->frameQty / config->framesPerProcess; i++){
            if(metadata->firstFrame[i] == PID) = -1;
        }
    pthread_mutex_unlock(&metadataMut);

    // clearTLBFromPID(PID);   TODO: Descomentar y arreglar nombre;

}

bool capiTermHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t pageQty = pt->pageQuantity;
    pthread_mutex_unlock(&pageTablesMut);

    for (int32_t i = pageQty; i > 0; i--){          // TODO: arreglar logica chancha y fea.
        swapInterface_erasePage(swapInterface, PID, i);

        pthread_mutex_lock(&pageTablesMut);
        if ((pt->entries)[i].present == true){
            uint32_t frame = (pt->entries)[i].frame;
            pthread_mutex_lock(&metadataMut);
                (metadata->entries)[frame].isFree = true;
            pthread_mutex_unlock(&metadataMut);
        }
        pthread_mutex_unlock(&pageTablesMut);
    }

    char *_PID = string_itoa(PID);
    dictionary_remove_and_destroy(pageTables, _PID, _destroyPageTable);

    t_packet *response = createPacket(OK, INITIAL_STREAM_SIZE);
    socket_sendPacket(socket, response);
    destroyPacket(response);

    pthread_mutex_lock(&logMut);
        log_info(memLogger, "Se desconecto el carpincho de PID %u.", PID);
    pthread_mutex_unlock(&logMut);

    // clearTLBFromPID(PID);   TODO: Descomentar y arreglar nombre;
    
    return true;
}

bool disconnectedHandler(t_packet* petition, int socket){
    t_packet *response = createPacket(OK, INITIAL_STREAM_SIZE);
    socket_sendPacket(socket, response);
    return false;
}

bool capiIDHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);

    t_pageTable *newPageTable = initializePageTable();
    char *_PID = string_itoa(PID);
    dictionary_put(pageTables, _PID, (void*) newPageTable);
    free(_PID);

    pthread_mutex_lock(&logMut);
        log_info(memLogger, "Se conecto un carpincho con PID #%u.", PID);
    pthread_mutex_unlock(&logMut);

    return true;
}

bool (*petitionHandlers[MAX_PETITIONS])(t_packet* petition, int socket) =
{
    capiIDHandler,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    memallocHandler,
    memfreeHandler,
    memreadHandler,
    memwriteHandler,
    suspendHandler,
    capiTermHandler,
    disconnectedHandler
};







/*

// UNUSED
int32_t getFrameForPage(uint32_t PID, int32_t page){
    --Buscar en TLB
    if(TLB_isPresent){
        return TLB_getFrame(PID, page);
    }

    --Buscar en Tabla de Paginas
    t_pageTable* table = dictionary_get(pageTables, PIDAsString);
    if(table->entries[page].present){
        return table->entries[page].frame;
    }

    --Buscar en Swap
    void* pageContents = swapInterface_loadPage(PID, page);
    int32_t newFrame = ramGetFreeFrame(memory);
    if(newFrame == -1){
        -- No hay frame libre
        int32_t victimFrame = ramGetVictim(memory);
        uint32_t victimPID = memory->metadata[victimFrame]->PID;
        uint32_t victimPage = memory->metadata[victimFrame]->page;
        
        if(memory->metadata[victimFrame]->modified){
            void* victimContents = ramGetFrameContents(victimFrame);
            swapInterface_savePage(victimPID, victimPage, victimContents); 
        }

        t_pageTable* victimTable = dictionary_get(pageTables, victimPIDAsString);
        victimTable->entries[victimPage].present = false;

        newFrame = victimFrame;
    }
    ramReplaceFrame(memory, newFrame, pageContents);
    table->entries[page].frame = newFrame;
    table->entries[page].present = true;
    return newFrame;    
}


// UNUSED, version vieja.
bool memallocHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);
    int32_t mallocSize = streamTake_INT32(petition->payload);
    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);

    if (pageTable_isEmpty(PID)){
        //Reserva de primera página si no tiene
        bool rc = createPage(PID);
        if(!rc){
            streamAdd_INT32(response->payload, 0);
            socket_sendPacket(socket, response);
            destroyPacket(response);
            pthread_mutex_lock(&mutex_log);
            log_info(memLogger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", PID, mallocSize);
            pthread_mutex_unlock(&mutex_log);
            return true;
        }

        //C: Creación de primer alloc, ocupa toda la primer página
        void* newPageContents = calloc(1, config->pageSize);
        t_heapMetadata firstAlloc = { .prevAlloc = NULL, .nextAlloc = NULL, .isFree = true };
        memcpy(newPageContents, &firstAlloc, 9);
        
        //C: Carga de la página generada en memoria principal
        int32_t newPageFrame = getFrameForPage(PID, page); // Page no declarado.
        ramReplaceFrame(ram, newPageFrame, newPageContents);
    }

    // bool found = false;      Unused
    int32_t thisAllocAddress = 0;
    t_heapMetadata* thisAlloc = heap_read(PID, thisAllocAddress, 9);
    int32_t thisAllocSize = thisAlloc->nextAlloc - thisAllocAddress - 9;

    //C: Búsqueda de alloc existente, libre, donde pueda entrar el nuevo
    while(thisAlloc->nextAlloc != NULL){
        if(thisAlloc->isFree){
            if(thisAllocSize == mallocSize || thisAllocSize > mallocSize + 9){
                thisAlloc->isFree = false;

                //C: Si sobra espacio, generar nuevo alloc libre nuevo en el espacio restante
                if(thisAllocSize > mallocSize){
                    t_heapMetadata* newMiddleMalloc = malloc(9);
                    int32_t newMiddleMallocAddress = thisAllocAddress + 9 + mallocSize;
                    newMiddleMalloc->prevAlloc = thisAllocAddress;
                    newMiddleMalloc->nextAlloc = thisAlloc->nextAlloc;
                    newMiddleMalloc->isFree = true;
                    thisAlloc->nextAlloc = newMiddleMallocAddress;
                    heap_write(PID, newMiddleMallocAddress, 9, newMiddleMalloc);
                    free(newMiddleMalloc);
                }

                //C: Asignación del alloc existente al nuevo y retorno de la dirección lógica
                heap_write(PID, thisAllocAddress, 9, thisAlloc);
                streamAdd_INT32(response->payload, thisAllocAddress + 9);
                socket_sendPacket(socket, response);
                destroyPacket(response);
                free(thisAlloc);
                return true;
            }
        }
        thisAllocAddress = thisAlloc->nextAlloc;
        free(thisAlloc);
        thisAlloc = heap_read(PID, thisAllocAddress, 9);
    }

    //Llegamos hasta aca, el alloc final en la ultima pagina
    int32_t thisAllocOffset = thisAllocAddress % config->pageSize;
    thisAllocSize = config->pageSize - thisAllocOffset - 9;

    //Si no alcanza el lugar para crear otro malloc en la misma pagina, creamos mas paginas
    if(thisAllocSize <= 9 + 1 || thisAllocSize - 9 - 1 < mallocSize){
        int newPages = 1 + (mallocSize - thisAllocSize + 9 + 1) / config->pageSize;
        // bool rc;         Unused
        for(int i = 0; i < newPages; i++){
            int32_t firstPage = 0;
            int32_t lastPage = 0;

            if(i == 0) { firstPage = createPage(PID); }
            // else { int32_t lastPage = createPage(PID); } UNUSED

            if(lastPage == -1 || firstPage == -1){
                if(lastPage == -1 && firstPage != -1){
                    for(int j = firstPage; j < lastPage; j++){
                        swapInterface_erasePage(swapInterface, PID, j);
                    }
                }
                streamAdd_INT32(response->payload, 0);
                socket_sendPacket(socket, response);
                destroyPacket(response);
                pthread_mutex_lock(&mutex_log);
                log_info(memLogger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", PID, mallocSize);
                pthread_mutex_unlock(&mutex_log);
                free(thisAlloc);
                return true;
            }
        }
    }
    
    thisAlloc->isFree = false;
    t_heapMetadata* newLastAlloc = malloc(9);
    int32_t newLastAllocAddress = thisAllocAddress + 9 + mallocSize;
    newLastAlloc->prevAlloc = thisAllocAddress;
    newLastAlloc->nextAlloc = thisAlloc->nextAlloc;
    newLastAlloc->isFree = true;
    thisAlloc->nextAlloc = newLastAllocAddress;
    heap_write(PID, newLastAllocAddress, 9, newLastAlloc);
    free(newLastAlloc);
    heap_write(PID, thisAllocAddress, 9, thisAlloc);
    streamAdd_INT32(response->payload, thisAllocAddress + 9);
    socket_sendPacket(socket, response);
    destroyPacket(response);
    free(thisAlloc);
    return true;
}
*/