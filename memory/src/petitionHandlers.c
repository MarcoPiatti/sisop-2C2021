#include <commons/string.h>
#include "memory.h"
#include "stdint.h"
#include "swapInterface.h"
#include "utils.h"


//  AUX
int32_t createPage(uint32_t PID){
    t_pageTable* table = getPageTable(PID, pageTables);
    int32_t pageNumber = pageTableAddEntry(table, 0);
    void* newPageContents = calloc(1, config->pageSize);
    if(swapInterface_savePage(swapInterface, PID, pageNumber, newPageContents)){
        return pageNumber;
    }
    return -1;
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
        ram_editFrame(ram, frame, startOffset, data, size);
        return;
    }

    //Si la data debe ser escrita entre varias paginas, las recorremos
    for(int i = startPage; i <= finishPage; i++){
        frame = getFrame(PID, i);

        // Si estamos en la primer pagina de la data, copiamos desde el offset inicial hasta el fin de pagina
        if(i == startPage){ 
            ram_editFrame(ram, frame, startOffset, data + dataOffset, config->pageSize - startOffset);
            dataOffset += config->pageSize - startOffset;
        }

        // Si estamos en la ultima pagina de la data, seguimos copiando desde el inicio de la pagina hasta el offset final
        else if (i == finishPage) {
            ram_editFrame(ram, frame, 0, data + dataOffset, finishOffset + 1);
            dataOffset += finishOffset + 1;
        }

        // Si estamos en una pagina que no es la primera o la ultima (una del medio) copiamos toda la pagina
        else{
            ram_editFrame(ram, frame, 0, data + dataOffset, config->pageSize);
            dataOffset += config->pageSize;
        }    
    }
}

uint32_t getLastAllocAddr(uint32_t PID) {
    uint32_t metadataAddr = 1;

    while(1){
        t_heapMetadata *currentMetadata = heap_read(PID, metadataAddr, sizeof(t_heapMetadata));

        if (currentMetadata->nextAlloc == 0){
            free(currentMetadata);
            return metadataAddr;
        }

        metadataAddr = currentMetadata->nextAlloc;
        free(currentMetadata);
    }
}

uint32_t getFreeAlloc(uint32_t PID, uint32_t size) {
    uint32_t metadataAddr = 1;

    while(1){
        t_heapMetadata *currentMetadata = heap_read(PID, metadataAddr, sizeof(t_heapMetadata));

        if (currentMetadata->nextAlloc == 0){
            free(currentMetadata);
            return 0;
        }
        
        uint32_t spaceToNextAlloc = currentMetadata->nextAlloc - metadataAddr - sizeof(t_heapMetadata);

        if (currentMetadata->isFree && spaceToNextAlloc > size){
            free(currentMetadata);
            return metadataAddr;
        }

        metadataAddr = currentMetadata->nextAlloc;
        free(currentMetadata);
    }
}

//  HANDLERS.
bool memallocHandler(t_packet* petition, int socket){
    uint32_t PID  = streamTake_UINT32(petition->payload);
    uint32_t size = streamTake_INT32(petition->payload);
    destroyPacket(petition);

    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);

    // Caso carpincho no tienen nada en memoria:
    if (pageTable_isEmpty(PID)) {
        int32_t newPageQty = div_roundUp(2 * sizeof(t_heapMetadata) + size + 1, config->pageSize);
        for (int i = 0; i < newPageQty; i++){
            if(createPage(PID) == -1) {
                streamAdd_INT32(response->payload, 0);
                socket_sendPacket(socket, response);
                destroyPacket(response);

                pthread_mutex_lock(&logMut);
                    log_info(logger, "No se pudo crear pagina para el carpincho de PID %u.", PID);
                pthread_mutex_unlock(&logMut);

                return false;
            }
        }
        t_heapMetadata first;
        first.isFree = true;
        first.nextAlloc = 1 + sizeof(t_heapMetadata) + size;    // El primer heapmetadata se crea en la direc 1 para dejar el 0 como 'NULL'.
        first.prevAlloc = 0;

        t_heapMetadata last;
        last.isFree = false;
        last.nextAlloc = 0;
        last.prevAlloc = 1;

        // Se guardan los nuevos primer y ultimo alloc a memoria.
        heap_write(PID, 1, sizeof(t_heapMetadata), &first);
        heap_write(PID, first.nextAlloc, sizeof(t_heapMetadata), &last);
        // Se deja espacio suficiente para el alloc y se continua la ejecucion, que deberia entrar por el siguiente caso.
    }

    // Caso hay alloc libre:
    if (getFreeAlloc(PID, size)){
        uint32_t foundAllocAddr = getFreeAlloc(PID, size);
        t_heapMetadata *found = heap_read(PID, foundAllocAddr, sizeof(t_heapMetadata));
        uint32_t distanceToNextAlloc = found->nextAlloc - foundAllocAddr - sizeof(t_heapMetadata);
        streamAdd_INT32(response->payload, foundAllocAddr);
        
        // Si solo hay lugar para la memoria pedida, no se crea un heapMetadata intermedio.
        if (distanceToNextAlloc <= size + sizeof(t_heapMetadata)){            
            // Actualizar heapMetadata en ram.
            found->isFree = false;
            heap_write(PID, foundAllocAddr, sizeof(t_heapMetadata), found);
            free(found);

            socket_sendPacket(socket, response);
            destroyPacket(response);

            return true;
        }

        // Si hay mas lugar, se crea un heapMetadata intermedio para evitar overallocar.
        t_heapMetadata *oldNextAlloc = heap_read(PID, found->nextAlloc, sizeof(t_heapMetadata));

        // Se crea nuevo alloc entre medio del encontrado y el siguiente.
        t_heapMetadata midAlloc = { .nextAlloc = found->nextAlloc, .prevAlloc = foundAllocAddr, .isFree = true };

        // Se actualizan los allocs que existian previamente.
        found->nextAlloc = foundAllocAddr + sizeof(t_heapMetadata) + size;
        found->isFree = false;
        oldNextAlloc->prevAlloc = found->nextAlloc;

        // Se actualiza la ram.
        heap_write(PID, foundAllocAddr, sizeof(t_heapMetadata), found);
        heap_write(PID, found->nextAlloc, sizeof(t_heapMetadata), &midAlloc);
        heap_write(PID, midAlloc.nextAlloc, sizeof(t_heapMetadata), oldNextAlloc);

        free(found);
        free(oldNextAlloc);

        socket_sendPacket(socket, response);
        destroyPacket(response);
        return true;
    }
    
    // Caso no hay alloc libre:
    uint32_t lastAllocAddr = getLastAllocAddr(PID);
    uint32_t lastAllocOffset = lastAllocAddr % config->pageSize;
    uint32_t freeSpaceInPage = config->pageSize - lastAllocOffset - sizeof(t_heapMetadata);
    uint32_t necessarySize = size + sizeof(t_heapMetadata);

    t_heapMetadata *lastAlloc = heap_read(PID, lastAllocAddr, sizeof(t_heapMetadata));

    // si hay espacio suficiente en la ult pagina, no se crea ninguna nueva.
    if(freeSpaceInPage >= necessarySize) {
        lastAlloc->isFree = false;
        lastAlloc->nextAlloc = lastAllocAddr + sizeof(t_heapMetadata) + size;

        t_heapMetadata newLastAlloc = { .nextAlloc = 0, .prevAlloc = lastAllocAddr, .isFree = false };

        heap_write(PID, lastAllocAddr, sizeof(t_heapMetadata), lastAlloc);
        heap_write(PID, lastAlloc->nextAlloc, sizeof(t_heapMetadata), &newLastAlloc);

        free(lastAlloc);

        streamAdd_INT32(response->payload, lastAllocAddr);
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

            return false;
        }
    }
    // Agregar siguiente y marcar como ocupado al anterior ultimo alloc.
    lastAlloc->isFree = false;
    lastAlloc->nextAlloc = lastAllocAddr + sizeof(t_heapMetadata) + size;

    // Crear nuevo ultimo alloc.
    t_heapMetadata newLastAlloc = { .nextAlloc = 0, .prevAlloc = lastAllocAddr, .isFree = false };

    // Actualizar ram.
    heap_write(PID, lastAllocAddr, sizeof(t_heapMetadata), lastAlloc);
    heap_write(PID, lastAlloc->nextAlloc, sizeof(t_heapMetadata), &newLastAlloc);

    free(lastAlloc);

    streamAdd_INT32(response->payload, lastAllocAddr);
    socket_sendPacket(socket, response);

    return 0;    

}

bool memfreeHandler(t_packet* petition, int socket){
    return true;    // placeholder
}

bool memreadHandler(t_packet* petition, int socket){
    return true;    // placeholder
}

bool memwriteHandler(t_packet* petition, int socket){
    return true;    // placeholder
}

bool capiTermHandler(t_packet* petition, int socket){
    return true;    // placeholder
}

bool disconnectedHandler(t_packet* petition, int socket){
    return true;    // placeholder
}

bool capiIDHandler(t_packet* petition, int socket){
    return true;    // placeholder
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
            log_info(logger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", PID, mallocSize);
            pthread_mutex_unlock(&mutex_log);
            return true;
        }

        //C: Creación de primer alloc, ocupa toda la primer página
        void* newPageContents = calloc(1, config->pageSize);
        t_heapMetadata firstAlloc = { .prevAlloc = NULL, .nextAlloc = NULL, .isFree = true };
        memcpy(newPageContents, &firstAlloc, sizeof(t_heapMetadata));
        
        //C: Carga de la página generada en memoria principal
        int32_t newPageFrame = getFrameForPage(PID, page); // Page no declarado.
        ramReplaceFrame(ram, newPageFrame, newPageContents);
    }

    // bool found = false;      Unused
    int32_t thisAllocAddress = 0;
    t_heapMetadata* thisAlloc = heap_read(PID, thisAllocAddress, sizeof(t_heapMetadata));
    int32_t thisAllocSize = thisAlloc->nextAlloc - thisAllocAddress - sizeof(t_heapMetadata);

    //C: Búsqueda de alloc existente, libre, donde pueda entrar el nuevo
    while(thisAlloc->nextAlloc != NULL){
        if(thisAlloc->isFree){
            if(thisAllocSize == mallocSize || thisAllocSize > mallocSize + sizeof(t_heapMetadata)){
                thisAlloc->isFree = false;

                //C: Si sobra espacio, generar nuevo alloc libre nuevo en el espacio restante
                if(thisAllocSize > mallocSize){
                    t_heapMetadata* newMiddleMalloc = malloc(sizeof(t_heapMetadata));
                    int32_t newMiddleMallocAddress = thisAllocAddress + sizeof(t_heapMetadata) + mallocSize;
                    newMiddleMalloc->prevAlloc = thisAllocAddress;
                    newMiddleMalloc->nextAlloc = thisAlloc->nextAlloc;
                    newMiddleMalloc->isFree = true;
                    thisAlloc->nextAlloc = newMiddleMallocAddress;
                    heap_write(PID, newMiddleMallocAddress, sizeof(t_heapMetadata), newMiddleMalloc);
                    free(newMiddleMalloc);
                }

                //C: Asignación del alloc existente al nuevo y retorno de la dirección lógica
                heap_write(PID, thisAllocAddress, sizeof(t_heapMetadata), thisAlloc);
                streamAdd_INT32(response->payload, thisAllocAddress + sizeof(t_heapMetadata));
                socket_sendPacket(socket, response);
                destroyPacket(response);
                free(thisAlloc);
                return true;
            }
        }
        thisAllocAddress = thisAlloc->nextAlloc;
        free(thisAlloc);
        thisAlloc = heap_read(PID, thisAllocAddress, sizeof(t_heapMetadata));
    }

    //Llegamos hasta aca, el alloc final en la ultima pagina
    int32_t thisAllocOffset = thisAllocAddress % config->pageSize;
    thisAllocSize = config->pageSize - thisAllocOffset - sizeof(t_heapMetadata);

    //Si no alcanza el lugar para crear otro malloc en la misma pagina, creamos mas paginas
    if(thisAllocSize <= sizeof(t_heapMetadata) + 1 || thisAllocSize - sizeof(t_heapMetadata) - 1 < mallocSize){
        int newPages = 1 + (mallocSize - thisAllocSize + sizeof(t_heapMetadata) + 1) / config->pageSize;
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
                log_info(logger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", PID, mallocSize);
                pthread_mutex_unlock(&mutex_log);
                free(thisAlloc);
                return true;
            }
        }
    }
    
    thisAlloc->isFree = false;
    t_heapMetadata* newLastAlloc = malloc(sizeof(t_heapMetadata));
    int32_t newLastAllocAddress = thisAllocAddress + sizeof(t_heapMetadata) + mallocSize;
    newLastAlloc->prevAlloc = thisAllocAddress;
    newLastAlloc->nextAlloc = thisAlloc->nextAlloc;
    newLastAlloc->isFree = true;
    thisAlloc->nextAlloc = newLastAllocAddress;
    heap_write(PID, newLastAllocAddress, sizeof(t_heapMetadata), newLastAlloc);
    free(newLastAlloc);
    heap_write(PID, thisAllocAddress, sizeof(t_heapMetadata), thisAlloc);
    streamAdd_INT32(response->payload, thisAllocAddress + sizeof(t_heapMetadata));
    socket_sendPacket(socket, response);
    destroyPacket(response);
    free(thisAlloc);
    return true;
}
*/