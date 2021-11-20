#include <commons/string.h>
#include "memory.h"
#include "stdint.h"
#include "pageTable.h"
#include "swapInterface.h"

int32_t createPage(uint32_t pid){
    t_pageTable* table = dictionary_get(pageTables, string_from_format("%i", pid));
    int32_t pageNumber = pageTableAddEntry(table, 0);
    void* newPageContents = calloc(1, config->pageSize);
    if(swapInterface_savePage(swapInterface, pid, pageNumber, newPageContents)){
        return pageNumber;
    }
    return -1;
}

int32_t getFrameForPage(uint32_t pid, int32_t page){
    /*
    --Buscar en TLB
    if(TLB_isPresent){
        return TLB_getFrame(pid, page);
    }

    --Buscar en Tabla de Paginas
    t_pageTable* table = dictionary_get(pageTables, pidAsString);
    if(table->entries[page].present){
        return table->entries[page].frame;
    }

    --Buscar en Swap
    void* pageContents = swapInterface_loadPage(pid, page);
    int32_t newFrame = ramGetFreeFrame(memory);
    if(newFrame == -1){
        -- No hay frame libre
        int32_t victimFrame = ramGetVictim(memory);
        uint32_t victimPid = memory->metadata[victimFrame]->pid;
        uint32_t victimPage = memory->metadata[victimFrame]->page;
        
        if(memory->metadata[victimFrame]->modified){
            void* victimContents = ramGetFrameContents(victimFrame);
            swapInterface_savePage(victimPid, victimPage, victimContents); 
        }

        t_pageTable* victimTable = dictionary_get(pageTables, victimPidAsString);
        victimTable->entries[victimPage].present = false;

        newFrame = victimFrame;
    }
    ramReplaceFrame(memory, newFrame, pageContents);
    table->entries[page].frame = newFrame;
    table->entries[page].present = true;
    return newFrame;    
    */
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
    int32_t startPage = logicAddress / config->pageSize;
    int32_t startOffset = logicAddress % config->pageSize;
    int32_t finishPage = (logicAddress + size - 1) / config->pageSize;
    int32_t finishOffset = (logicAddress + size - 1) % config->pageSize;
    
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

void heap_write(uint32_t pid, int32_t logicAddress, int size, void* data){
    int32_t startPage = logicAddress / config->pageSize;
    int32_t startOffset = logicAddress % config->pageSize;
    int32_t finishPage = (logicAddress + size - 1) / config->pageSize;
    int32_t finishOffset = (logicAddress + size - 1) % config->pageSize;

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

bool memallocHandler(t_packet* petition, int socket){
    uint32_t pid = streamTakeU_INT32(petition->payload);
    int32_t mallocSize = streamTake_INT32(petition->payload);
    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);

    if (pageTable_isEmpty(pid)){
        //Reserva de primera página si no tiene
        bool rc = createPage(pid);
        if(!rc){
            streamAdd_INT32(response->payload, 0);
            socket_sendPacket(socket, response);
            destroyPacket(response);
            pthread_mutex_lock(&mutex_log);
            log_info(logger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", pid, mallocSize);
            pthread_mutex_unlock(&mutex_log);
            return true;
        }

        void* newPageContents = calloc(1, config->pageSize);
        t_heapMetadata firstAlloc = { .prevAlloc = NULL, .nextAlloc = NULL, .isFree = true };
        memcpy(newPageContents, &firstAlloc, sizeof(t_heapMetadata));
        
        int32_t newPageFrame = getFrameForPage(pid, page);
        ramReplaceFrame(memory, newPageFrame, newPageContents);
    }

    bool found = false;
    int32_t thisAllocAddress = 0;
    t_heapMetadata* thisAlloc = heap_read(pid, thisAllocAddress, sizeof(t_heapMetadata));
    int32_t thisAllocSize = thisAlloc->nextAlloc - thisAllocAddress - sizeof(t_heapMetadata);
    while(thisAlloc->nextAlloc != NULL){
        if(thisAlloc->isFree){
            if(thisAllocSize == mallocSize || thisAllocSize > mallocSize + sizoef(t_heapMetadata)){
                thisAlloc->isFree = false;
                if(thisAllocSize > mallocSize){
                    t_heapMetadata* newMiddleMalloc = malloc(sizeof(t_heapMetadata));
                    int32_t newMiddleMallocAddress = thisAllocAddress + sizeof(t_heapMetadata) + mallocSize;
                    newMiddleMalloc->prevAlloc = thisAllocAddress;
                    newMiddleMalloc->nextAlloc = thisAlloc->nextAlloc;
                    newMiddleMalloc->isFree = true;
                    thisAlloc->nextAlloc = newMiddleMallocAddress;
                    heap_write(pid, newMiddleMallocAddress, sizeof(t_heapMetadata), newMiddleMalloc);
                    free(newMiddleMalloc);
                }
                heap_write(pid, thisAllocAddress, sizeof(t_heapMetadata), thisAlloc);
                streamAdd_INT32(response->payload, thisAllocAddress + sizeof(t_heapMetadata));
                socket_sendPacket(socket, response);
                destroyPacket(response);
                free(thisAlloc);
                return true;
            }
        }
        thisAllocAddress = thisAlloc->nextAlloc;
        free(thisAlloc);
        thisAlloc = heap_read(pid, thisAllocAddress, sizeof(t_heapMetadata));
    }

    //Llegamos hasta aca, el alloc final en la ultima pagina
    int32_t thisAllocOffset = thisAllocAddr % config->pageSize;
    thisAllocSize = config->pageSize - thisAllocOffset - sizeof(t_HeapMetadata);

    //Si no alcanza el lugar para crear otro malloc en la misma pagina, creamos mas paginas
    if(thisAllocSize <= sizeof(t_HeapMetadata) + 1 || thisAllocSize - sizeof(t_HeapMetadata) - 1 < mallocSize){
        int newPages = 1 + (mallocSize - thisAllocSize + sizeof(t_HeapMetadata) + 1) / config->pageSize;
        bool rc;
        for(int i = 0; i < newPages; i++){
            int32_t firstPage = 0;
            int32_t lastPage = 0;

            if(i == 0) { firstPage = createPage(pid); }
            else { int32_t lastPage = createPage(pid); }

            if(lastPage == -1 || firstPage == -1){
                if(lastPage == -1 && firstPage != -1){
                    for(int j = firstPage; j < lastPage; j++){
                        swapInterface_erasePage(pid, j);
                    }
                }
                streamAdd_INT32(response->payload, 0);
                socket_sendPacket(socket, response);
                destroyPacket(response);
                pthread_mutex_lock(&mutex_log);
                log_info(logger, "Proceso %u: No pudo asignar %i bytes, no habia lugar para mas paginas", pid, mallocSize);
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
    heap_write(pid, newLastAllocAddress, sizeof(t_heapMetadata), newLastAlloc);
    free(newLastAlloc);
    heap_write(pid, thisAllocAddress, sizeof(t_heapMetadata), thisAlloc);
    streamAdd_INT32(response->payload, thisAllocAddress + sizeof(t_heapMetadata));
    socket_sendPacket(socket, response);
    destroyPacket(response);
    free(thisAlloc);
    return true;
}

bool memfreeHandler(t_packet* petition, int socket){
    ;
}

bool memreadHandler(t_packet* petition, int socket){
    ;
}

bool memwriteHandler(t_packet* petition, int socket){
    ;
}

bool capiTermHandler(t_packet* petition, int socket){
    ;
}

bool disconnectedHandler(t_packet* petition, int socket){
    ;
}

bool capiIDHandler(t_packet* petition, int socket){
    ;
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
}