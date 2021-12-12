#include <commons/string.h>
#include "memory.h"
#include "stdint.h"
#include "swapInterface.h"
#include "utils.h"
#include <commons/memory.h>
#include "tlb.h"


//  AUX

bool validMallocAddress(uint32_t PID, uint32_t addr){

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t maxPags = pt->pageQuantity;
    pthread_mutex_unlock(&pageTablesMut);

    uint32_t maxAddr = maxPags * config->pageSize;

    if (addr < maxAddr) return true;
    return false;
}

int32_t createPage(uint32_t PID){
    

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable* table = getPageTable(PID, pageTables);
    pthread_mutex_unlock(&pageTablesMut);
    
    int32_t pageNumber = pageTableAddEntry(table, 0);

    pthread_mutex_lock(&logMut);
        log_debug(logger, "Creando pagina %i, para carpincho de PID %u.", pageNumber, PID);
    pthread_mutex_unlock(&logMut);

    void* newPageContents = calloc(1, config->pageSize);
    if(swapInterface_savePage(swapInterface, PID, pageNumber, newPageContents)){
        free(newPageContents);
        return pageNumber;
    }
    free(newPageContents);
    return -1;
}

void deleteLastPages(uint32_t PID, uint32_t lastAllocAddr){
    
    uint32_t firstToDelete = (lastAllocAddr + 9) / config->pageSize;

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t lastToDelete = pt->pageQuantity - 1;
    pthread_mutex_unlock(&pageTablesMut);
    
    for (uint32_t i = lastToDelete; i > firstToDelete; i--){
        pthread_mutex_lock(&logMut);
        log_debug(logger, "Borrando pagina %i, del carpincho de PID %u.", i, PID);
        pthread_mutex_unlock(&logMut);
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
        log_debug(logger, "Paquete MEMALLOC recibido");
    pthread_mutex_unlock(&logMut);

    uint32_t PID  = streamTake_UINT32(petition->payload);
    uint32_t size = streamTake_INT32(petition->payload);

    pthread_mutex_lock(&logMut);
        log_debug(logger, "Haciendo alloc de %u, bytes para carpincho de PID %u.", size, PID);
    pthread_mutex_unlock(&logMut);

    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);

    // Caso carpincho no tienen nada en memoria:
    if (pageTable_isEmpty(PID)) {
        if(createPage(PID) == -1) {
            streamAdd_INT32(response->payload, 0);
            socket_sendPacket(socket, response);
            destroyPacket(response);

            pthread_mutex_lock(&logMut);
                log_debug(logger, "No se pudo crear pagina para el carpincho de PID %u.", PID);
            pthread_mutex_unlock(&logMut);

            return true;
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
    uint32_t lastAllocOffset = (currentAllocAddr + 9) % config->pageSize;
    uint32_t freeSpaceInPage = config->pageSize - lastAllocOffset;
    uint32_t necessarySize = size + 9 + 1;

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
                log_info(logger, "No se pudo crear pagina para el carpincho de PID %u.", PID);
            pthread_mutex_unlock(&logMut);

            deleteLastPages(PID, currentAllocAddr);

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
    destroyPacket(response);

    return true;    

}

bool memfreeHandler(t_packet* petition, int clientSocket){
    uint32_t PID = streamTake_UINT32(petition->payload);
    int32_t addr = streamTake_INT32(petition->payload);
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: mate_memfree (direccion %i)", PID, addr);
    pthread_mutex_unlock(&mutex_log);
    
    //Si la direccion es invalida corta y retorna error al carpincho
    if(!validMallocAddress(PID, addr)){
        t_packet* response = createPacket(ERROR, 0);
        socket_sendPacket(clientSocket, response);
        free(response);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: trato de hacer free de una direccion invalida", PID);
        pthread_mutex_unlock(&mutex_log);

        return true;
    }

    int32_t thisAllocPtr, rightAllocPtr, leftAllocPtr, finalAllocPtr;
    t_heapMetadata *thisAlloc, *rightAlloc, *leftAlloc, *finalAlloc;
    
    thisAllocPtr = addr - 9;
    thisAlloc = heap_read(PID, thisAllocPtr, 9);
    finalAllocPtr = thisAllocPtr;
    finalAlloc = thisAlloc;

    //Nuestro alloc absorbe al derecho si el derecho esta libre
    if(thisAlloc->nextAlloc != NULL){
        rightAllocPtr = thisAlloc->nextAlloc;
        rightAlloc = heap_read(PID, rightAllocPtr, 9);
        if(rightAlloc->isFree){
            thisAlloc->nextAlloc = rightAlloc->nextAlloc;
        }
    }

    //El alloc izquierdo absorbe a nuestro alloc si el izquierdo esta libre
    leftAllocPtr = thisAlloc->prevAlloc;
    leftAlloc = heap_read(PID, leftAllocPtr, 9);
    if(leftAlloc->isFree){
        leftAlloc->nextAlloc = thisAlloc->nextAlloc;
        finalAlloc = leftAlloc;
        finalAllocPtr = leftAllocPtr;
        heap_write(PID, leftAllocPtr, 9, leftAlloc);
    }

    thisAlloc->isFree = true;
    heap_write(PID, thisAllocPtr, 9, thisAlloc);

    //Se liberan paginas sobrantes si esta al final.
    if(finalAlloc->nextAlloc == NULL){

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: Se borran paginas libres en el free", PID);
        pthread_mutex_unlock(&mutex_log);

        int32_t firstFreeByte = finalAllocPtr + 9;
        int32_t pageToDelete = 1 + firstFreeByte / config->pageSize;
        
        pthread_mutex_lock(&pageTablesMut);
            t_pageTable *pt = getPageTable(PID, pageTables);
            int totalPages = pt->pageQuantity;
        pthread_mutex_unlock(&pageTablesMut);

        for(int i = totalPages-1; i >= pageToDelete; i--){
            if(isPresent(PID, i)){
                int frame = pageTable_getFrame(PID, i);
                clearFrameMetadata(frame);
                dropEntry(PID, i);
            }
            pageTable_destroyLastEntry(pt);
            swapInterface_erasePage(swapInterface, PID, i);
        }
    } else {
        t_heapMetadata *rightmostAlloc = heap_read(PID, finalAlloc->nextAlloc, 9);
        rightmostAlloc->prevAlloc = finalAllocPtr;
        heap_write(PID, finalAlloc->nextAlloc, 9, rightmostAlloc);
        free(rightmostAlloc);
    }
    
    free(thisAlloc);
    free(leftAlloc);
    free(rightAlloc);
    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);

    pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %u: hizo free en la direccion %i", PID, addr);
    pthread_mutex_unlock(&mutex_log);

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

    if(metadata->firstFrame){
        pthread_mutex_lock(&metadataMut);
            for (uint32_t i = 0; i < config->frameQty / config->framesPerProcess; i++){
                if(metadata->firstFrame[i] == PID) metadata->firstFrame[i] = -1;
            }
        pthread_mutex_unlock(&metadataMut);
    }
    
    freeProcessEntries(PID);

    return true;
}

bool capiTermHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);

    pthread_mutex_lock(&pageTablesMut);
        t_pageTable *pt = getPageTable(PID, pageTables);
        uint32_t pageQty = pt->pageQuantity;
    pthread_mutex_unlock(&pageTablesMut);

    for (int32_t i = pageQty - 1; i >= 0; i--){
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
    free(_PID);

    t_packet *response = createPacket(OK, INITIAL_STREAM_SIZE);
    socket_sendPacket(socket, response);
    destroyPacket(response);

    pthread_mutex_lock(&logMut);
        log_info(logger, "Se desconecto el carpincho de PID %u.", PID);
    pthread_mutex_unlock(&logMut);

    sigUsr1HandlerTLB(0);

    freeProcessEntries(PID);
    
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
    pthread_mutex_lock(&pageTablesMut);
    dictionary_put(pageTables, _PID, (void*) newPageTable);
    pthread_mutex_unlock(&pageTablesMut);
    free(_PID);

    pthread_mutex_lock(&logMut);
        log_info(logger, "Se conecto un carpincho con PID #%u.", PID);
    pthread_mutex_unlock(&logMut);

    socket_sendHeader(socket, ID_MEMORIA);

    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(socket, response);
    destroyPacket(response);

    return true;
}

bool (*petitionHandlers[MAX_PETITIONS])(t_packet* petition, int socket) = {
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
bool memfreeHandler(t_packet* petition, int socket){
    uint32_t PID = streamTake_UINT32(petition->payload);
    int32_t addr = streamTake_INT32(petition->payload);
    t_packet *response;

    pthread_mutex_lock(&logMut);
        log_info(logger, "Llego MEMFREE: pid %u, addr: %i.", PID, addr);
    pthread_mutex_unlock(&logMut);



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
*/