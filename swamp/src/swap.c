#include "swap.h"

t_swapFile* pidExists(uint32_t pid){
    bool hasProcess(void* elem){
        return swapFile_hasPid((t_swapFile*)elem, pid);
    };
    t_swapFile* file = list_find(swapFiles, hasProcess);
    return file;
}

// Se fija si en un dado swapFile hay un "chunk" libre (acorde a asignacion fija)
bool hasFreeChunk(t_swapFile* sf){
    bool hasFreeChunk = false;
    for(int i = 0; i < sf->maxPages; i += swapConfig->maxFrames)
        if(!sf->entries[i].used){
            hasFreeChunk = true;
            break;
        }
    return hasFreeChunk;
}

// Encuentra el indice de comienzo de un chunk libre (acorde a asignacion fija)
int findFreeChunk(t_swapFile* sf){
    for(int i = 0; i < sf->maxPages; i += swapConfig->maxFrames){
        if(!(sf->entries[i].used)){
            return i;
        }
    }
    return -1;
}

// Encuentra el indice de comienzo del chunk donde se encuentra PID (acorde a asignacion fija)
int getChunk(t_swapFile* sf, uint32_t pid){
    int i = 0;
    while(sf->entries[i].pid != pid){
        if(i >= sf->maxPages) return -1;
        i += swapConfig->maxFrames;
    }
    if(!sf->entries[i].used) return -1;
    return i;
}

// Algoritmo de asignacion fija. 
// Si el proceso existe en algun archivo, y hay lugar para una pagina mas en su chunk, se asigna.
// Si el proceso existe en algun archivo, y no hay mas lugar, no se asigna.
// Si el proceso NO existe en ningun archivo, pero hay un archivo con un chunk disponible, se asigna.
// Si el proceso NO existe en ningun archivo, y no hay ningun archivo con un chunk disponible, no se asigna.
bool fija(uint32_t pid, int32_t page, void* pageContent){
    t_swapFile* file = pidExists(pid);
    bool _hasFreeChunk(void* elem){
        return hasFreeChunk((t_swapFile*)elem);
    };
    if (file == NULL)
        file = list_find(swapFiles, _hasFreeChunk);
    if (file == NULL)
        return false;

    int assignedIndex = swapFile_getIndex(file, pid, page);
    if (assignedIndex == -1){
        int base = getChunk(file, pid);
        if (base == -1) base = findFreeChunk(file);
        if (base == -1) return false;
        int offset = swapFile_countPidPages(file, pid);
        if (offset == swapConfig->maxFrames) return false;
        assignedIndex = base + offset;
    }
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Archivo %s: se almaceno la pagina %i del proceso %i en el indice %i", file->path, page, pid, assignedIndex);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

//Algoritmo de Asignacion global.
// Si el proceso existe en algun archivo, y hay mas lugar en ese archivo, se asigna.
// Si el proceso existe en algun archivo, y NO hay mas lugar en ese archivo, no se asigna.
// Si el proceso NO existe en ningun archivo, pero hay un archivo con un lugar disponible, se asigna.
// Si el proceso NO existe en ningun archivo, y NO hay ningun archivo con un lugar disponible, no se asigna.
bool global(uint32_t pid, int32_t page, void* pageContent){
    t_swapFile* file = pidExists(pid);
    
    void* hasMoreFreeIndexes(void* elem1, void* elem2){
        if(swapFile_countFreeIndexes((t_swapFile*)elem1) >= swapFile_countFreeIndexes((t_swapFile*)elem2))
            return elem1;
        else return elem2;
    };
    if (file == NULL)
        file = list_get_maximum(swapFiles, hasMoreFreeIndexes);
    if (file == NULL)
        return false;
    int assignedIndex = swapFile_getIndex(file, pid, page);
    if (assignedIndex == -1){
        if (swapFile_isFull(file)) return false;
        assignedIndex = swapFile_findFreeIndex(file);
    }
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Archivo %s: se almaceno la pagina %i del proceso %i en el indice %i", file->path, page, pid, assignedIndex);
    pthread_mutex_unlock(&mutex_log);

    return true;
}

int main(){
    logger = log_create("./cfg/swamp.log", "Swap", true, LOG_LEVEL_TRACE);
    pthread_mutex_init(&mutex_log, NULL);

    swapConfig = getswapConfig("./cfg/swamp.config");
    swapFiles = list_create();
    for(int i = 0; swapConfig->swapFiles[i]; i++)
        list_add(swapFiles, swapFile_create(swapConfig->swapFiles[i], swapConfig->fileSize, swapConfig->pageSize));

    int serverSocket = createListenServer(swapConfig->swapIP, swapConfig->swapPort);
    while(1){
        int memorySocket = getNewClient(serverSocket);
        swapHeader asignType = socket_getHeader(memorySocket);
        if (asignType == ASIG_FIJA) asignacion = fija;
        else if (asignType == ASIG_GLOBAL) asignacion = global;

        t_packet* petition;
        while(1){
            petition = socket_getPacket(memorySocket);
            if(petition == NULL){
                if(!retry_getPacket(memorySocket, &petition)){
                    close(memorySocket);
                    break;
                }
            }
            for(int i = 0; i < swapConfig->delay; i++){
                usleep(1000);
            }
            petitionHandler[petition->header](petition, memorySocket);
            destroyPacket(petition);
        }
    }
    return 0;
}