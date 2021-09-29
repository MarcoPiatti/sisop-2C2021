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

    //marcamos la pagina como presente en la tabla de paginas, anotamos el nuevo frame y actualizamos TLB
    pageTable_setPresent(pageTable, pid, page, true);
    pageTable_setFrame(pageTable, pid, page, frame);
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

void* thread_clientHandlerFunc(void* args){
    int clientSocket = (int)args;
    socket_sendHeader(clientSocket, ID_MEMORIA);
    t_packet* petition;
    bool keepServing = true;
    while(keepServing){
        petition = socket_getPacket(clientSocket);

        pthread_mutex_lock(&mutexMemoria);
        keepServing = petitionHandlers[petition->header](clientSocket, petition);
        pthread_mutex_unlock(&mutexMemoria);
        
        destroyPacket(petition);
    }
    
    return 0;
}

int main(){
    //Inicializa Logger
    logger = log_create("./cfg/memory.log", "Memoria", true, LOG_LEVEL_TRACE);
    pthread_mutex_init(&mutex_log, NULL);

    //Obtiene datos de config
    memoryConfig = getMemoryConfig("./cfg/memory.config");

    //Obtiene tipo de asignacion de memoria segun config
    swapHeader asignSwap;
    void(*MMUAssign)(t_ram*, uint32_t, int*, int*);
    if(!strcmp(memoryConfig->tipoAsignacion, "FIJA")){
        asignSwap = ASIGN_FIJO;
        MMUAssign = fixed;
    }
    else if(!strcmp(memoryConfig->tipoAsignacion, "GLOBAL")){
        asignSwap = ASIGN_GLOBAL;
        MMUAssign = global;
    }

    //Obtiene algoritmo de seleccion de victimas de MMU segun config
    int(*MMUVictim)(t_ram*, int, int);
    if(!strcmp(memoryConfig->algoritmoMMU, "LRU")){
        MMUVictim = LRU;
    }
    else if(!strcmp(memoryConfig->algoritmoMMU, "CLOCK-M")){
        MMUVictim = CLOCK_M;
    }

    //Crea interfaz al swap a partir de datos sacados de config
    swapInterface = swapInterface_create(memoryConfig->swapIp
                                        ,memoryConfig->swapPort
                                        ,memoryConfig->pageSize
                                        ,asignSwap);
    
    //Crea una ram a partir de datos sacados de config
    ram = createRam (memoryConfig->ramSize
                    ,memoryConfig->pageSize
                    ,MMUVictim
                    ,MMUAssign
                    ,memoryConfig->maxFramesMMU);

    //Obtiene algoritmo de seleccion de victimas de TLB segun config 
    void(*TLBVictim)(t_TLB*, t_TLBEntry*);
    if(!strcmp(memoryConfig->algoritmoTLB, "FIFO")){
        TLBVictim = fifo;
    }
    else if(!strcmp(memoryConfig->algoritmoTLB, "LRU")){
        TLBVictim = lru;
    }

    //Crea TLB a partir de datos sacados de config
    tlb = createTLB(memoryConfig->TLBSize, TLBVictim);

    //Crea tabla de paginas
    pageTable = pageTable_create();

    //Inicializo mutex gigante (DESPUES SACARLO Y HACERLO BIEN)
    pthread_mutex_init(&mutexMemoria, NULL);

    //Inicializa server y empieza a recibir clientes
    int memoryServer = createListenServer(memoryConfig->memoryIp, memoryConfig->memoryPort);
    while(1){
        runListenServer(memoryServer, thread_clientHandlerFunc);
    }

    return 0;
}