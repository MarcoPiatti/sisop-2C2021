#include <stdint.h>
#include <commons/string.h>
#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include "swapInterface.h"
#include "utils.h"

// TODO: coordinar ram, metadata y pagetables.
// TODO: cambiar metadata->firstFrame cuando se inicializa (o finaliza) un proceso en memoria.
// TODO: asignar algoritmo y asignacion a variables algoritmo y asignacion en main.

int main(){

    initalizeMemMutex();

    // Initialize memLogger.
    memLogger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
    
    // Load and validate config
    config = getMemoryConfig("./memory.cfg");
    validateConfg(config, memLogger);
    
    ram = initializeMemory(config);
    metadata = initializeMemoryMetadata(config);
    pageTables = dictionary_create();

    swapHeader tipoAsig = strcmp(config->assignmentType, "FIJA") ? ASIG_GLOBAL : ASIG_FIJA;
    char *swapPort = string_itoa(config->swapPort);
    swapInterface = swapInterface_create(config->swapIP, swapPort, config->pageSize, tipoAsig);


    char* port = string_itoa(config->port);
    int serverSocket = createListenServer(config->ip, port);
    
    while(1){
        runListenServer(serverSocket, petitionHandler);
    }

    destroyMemoryMetadata(metadata);
    destroyMemoryConfig(config);
    dictionary_destroy_and_destroy_elements(pageTables, _destroyPageTable);
    log_destroy(memLogger);
    

    // TODO: Eliminar estas variables auxiliares (cambiar tipo de config->puerto a char).
    free(port);
    free(swapPort);

    destroyMemMutex();
    return 0;
}

void *petitionHandler(void *_clientSocket){
    int clientSocket = (int) _clientSocket;
    bool keepServing = true;
    socket_sendHeader(clientSocket, ID_MEMORIA);
    while(keepServing){
        t_packet *petition = socket_getPacket(clientSocket);
        keepServing = petitionHandlers[petition->header](petition, clientSocket);
        destroyPacket(petition);
    }
    return 0; // Para cumplir con el tipo.
}

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config){
    t_memoryMetadata *newMetadata = malloc(sizeof(t_memoryMetadata));
    newMetadata->entryQty = config->frameQty;
    newMetadata->counter = 0;
    newMetadata->entries = calloc(newMetadata->entryQty, sizeof(t_frameMetadata));

    if (strcmp(config->assignmentType, "FIJA")){
        uint32_t blockQty = config->frameQty / config->framesPerProcess;
        newMetadata->firstFrame = calloc(blockQty, sizeof(uint32_t));
        memset(newMetadata->firstFrame, 0, sizeof(uint32_t) * blockQty);        
    } else newMetadata->firstFrame = NULL;

    for (int i = 0; i < newMetadata->entryQty; i++){
        ((newMetadata->entries)[i])->isFree = 1;
        ((newMetadata->entries)[i])->timeStamp = 0;
    }

    return newMetadata; 
}

void destroyMemoryMetadata(t_memoryMetadata *meta){
    free(meta->firstFrame);
    free(meta->entries);
    free(meta);
}

t_memory *initializeMemory(t_memoryConfig *config){
    t_memory *newMemory = malloc(sizeof(t_memory));
    newMemory->memory = calloc(1,config->size);
    newMemory->config = config;

    return newMemory;
}

void destroyMemory(t_memory* mem){
    free(mem->memory);
    free(mem);
}

int32_t getFreeFrame(uint32_t start, uint32_t end){
    for(uint32_t i = start; i < end; i++){
        if(isFree(i)) return i;
    }
    return -1;
}

void readFrame(t_memory *mem, uint32_t frame, void *dest){
    void *frameAddress = ram_getFrame(mem, frame);
    pthread_mutex_lock(&ramMut);
        memcpy(dest, frameAddress, mem->config->pageSize);
    pthread_mutex_unlock(&ramMut);
}

void writeFrame(t_memory *mem, uint32_t frame, void *from){
    void *frameAddress = ram_getFrame(mem, frame);
    pthread_mutex_lock(&ramMut);
        memcpy(frameAddress, from, mem->config->pageSize);
    pthread_mutex_unlock(&ramMut);
}   

void *ram_getFrame(t_memory *mem, uint32_t frame){
    pthread_mutex_lock(&ramMut);
        void* ptr = mem->memory + frame * config->pageSize;
    pthread_mutex_unlock(&ramMut);
    return ptr;
}

int32_t getPage(uint32_t address, t_memoryConfig *cfg){
    return address / cfg->pageSize;
}

int32_t getOffset(uint32_t address, t_memoryConfig *cfg){
    return address % cfg->pageSize;
}

void fijo(int32_t *start, int32_t *end, uint32_t PID){
    pthread_mutex_lock(&metadataMut);
        for (uint32_t i = 0; i < config->frameQty / config->framesPerProcess; i++){
            if ((metadata->firstFrame)[i] == PID){
                *start = i * config->framesPerProcess;
                break;
            }
        }
    pthread_mutex_unlock(&metadataMut);
    
    *end = *start + config->framesPerProcess;
}

void global(int32_t *start, int32_t *end, uint32_t PID){
    *start = 0;
    *end = config->frameQty;
}

int32_t getFrame(uint32_t PID, uint32_t pageN){

    /* TODO: Integrar con TLB:
        * Buscar en TLB antes de tabla de paginas.
        * Actualizar TLB cuando se swapean paginas.
    */

    // Si esta presente retorna el numero de frame.
    t_pageTable* pt = getPageTable(PID, pageTables);
    pthread_mutex_lock(&pageTablesMut);
    if (((pt->entries)[pageN]).present) {
        uint32_t frame = ((pt->entries)[pageN]).frame;
        
        pthread_mutex_unlock(&pageTablesMut);
        return frame;
    }
    pthread_mutex_unlock(&pageTablesMut);

    // Si no esta presente hay que traerla de swap.
    return swapPage(PID, pageN);

}

bool isPresent(uint32_t PID, uint32_t page){
    pthread_mutex_lock(&pageTablesMut);
        t_pageTable* pt = getPageTable(PID, pageTables);
        bool present = (pt->entries)[page].present;
    pthread_mutex_unlock(&pageTablesMut);
    return present;
}

uint32_t swapPage(uint32_t PID, uint32_t page){

    uint32_t start, end;
    asignacion(&start, &end, PID);

    uint32_t victima = algoritmo(start, end);

    return replace(victima, PID, page);

}

uint32_t replace(uint32_t victim, uint32_t PID, uint32_t page){
    // Traer pagina pedida de swap.
    void *pageFromSwap = swapInterface_loadPage(swapInterface, PID, page);

    // Chequear que se haya podido traer.
    if (!pageFromSwap){
        pthread_mutex_lock(&logMut);
            log_error(memLogger, "No se pudeo cargar pagina #%i del PID #%i", page, PID);
        pthread_mutex_unlock(&logMut);        
        return -1;
    }

    // Si el frame no esta libre se envia a swap la pagina que lo ocupa. 
    // Esto es para que replace() se pueda utilizar tanto para cargar paginas a frames libres como para reemplazar.
    if (! isFree(victim)){
        // Enviar pagina reemplazada a swap.
        pthread_mutex_lock(&metadataMut);
            uint32_t victimPID  = (metadata->entries)[victim]->PID;
            uint32_t victimPage = (metadata->entries)[victim]->page;
        pthread_mutex_lock(&metadataMut);

        swapInterface_savePage(swapInterface, victimPID, victimPage, ram_getFrame(ram, victim));
        // Modificar tabla de paginas del proceso cuya pagina fue reemplazada.
        pthread_mutex_lock(&pageTablesMut);
            t_pageTable *ptReemplazado = getPageTable(victimPID, pageTables);
            (ptReemplazado->entries)[victimPage].present = false;
            (ptReemplazado->entries)[victimPage].frame = -1;
        pthread_mutex_unlock(&pageTablesMut);

    }

    // Escribir pagina traida de swap a memoria.
    writeFrame(ram, victim, pageFromSwap);
    // Modificar tabla de paginas del proceso cuya pagina entra a memoria.
    t_pageTable *ptReemplaza = getPageTable(PID, pageTables);
    pthread_mutex_lock(&pageTablesMut);
        (ptReemplaza->entries)[page].present = true;
        (ptReemplaza->entries)[page].frame = victim;
    pthread_mutex_unlock(&pageTablesMut);
    // Modificar frame metadata.
    pthread_mutex_lock(&metadataMut);
        (metadata->entries)[victim]->page = page;
        (metadata->entries)[victim]->PID = PID;
        (metadata->entries)[victim]->isFree = false;
    pthread_mutex_unlock(&metadataMut);

    return victim;
}

bool isFree(uint32_t frame){
    pthread_mutex_lock(&metadataMut);
        bool free = (metadata->entries)[frame]->isFree;
    pthread_mutex_unlock(&metadataMut);
    return free;
}
uint32_t getFrameTimestamp(uint32_t frame){
    pthread_mutex_lock(&metadataMut);
        uint32_t ts = (metadata->entries)[frame]->timeStamp;
    pthread_mutex_lock(&metadataMut);
    return ts;
}

uint32_t LRU(uint32_t start, uint32_t end){
    
    int32_t frame = getFreeFrame(start, end);
    if(frame != -1) return frame;

    pthread_mutex_lock(&metadataMut);
        int32_t min = metadata->counter;
    pthread_mutex_unlock(&metadataMut);

    for(uint32_t i = start; i < end; i++){
        if (getFrameTimestamp(i) < min){
            frame = i;
            min = getFrameTimestamp(i);
        }
    }
    return frame;
}

void ram_editFrame(t_memory *mem, uint32_t offset, uint32_t frame, void *from, uint32_t size){
    void *frameAddress = ram_getFrame(mem, frame);
    void *dest = frameAddress + offset;
    
    pthread_mutex_lock(&ramMut);
        memcpy(dest, from, size);
    pthread_mutex_unlock(&ramMut);
}

void updateTimestamp(uint32_t frame) {
    pthread_mutex_lock(&metadataMut);
        metadata->counter += 1;
        (metadata->entries)[frame]->timeStamp = metadata->counter;
    pthread_mutex_unlock(&metadataMut);
}

void initalizeMemMutex(){
    pthread_mutex_init(&logMut, NULL);
    pthread_mutex_init(&ramMut, NULL);
    pthread_mutex_init(&metadataMut, NULL);
    pthread_mutex_init(&pageTablesMut, NULL);    
}

void destroyMemMutex(){
    pthread_mutex_destroy(&logMut);
    pthread_mutex_destroy(&ramMut);
    pthread_mutex_destroy(&metadataMut);
    pthread_mutex_destroy(&pageTablesMut);
}























/*

// Unused, reemplazado por heapRead()
void memread(uint32_t bytes, uint32_t address, int PID, void *destination){
    uint32_t firstPage = getPage(address, config);
    uint32_t offset = getOffset(address, config);

    uint32_t toRead = min(bytes, config->pageSize - offset);
    uint32_t fullPages = (bytes - toRead) / config->pageSize;

    uint32_t firstFrame = getFrame(PID, firstPage);
    void *aux = destination;

    // Leer "pedacito" de memoria en el final de una pag.
    memcpy(aux, ram_getFrame(ram, firstFrame), toRead);
    aux += toRead;

    // Leer frames del medio completos.
    toRead = config->pageSize;
    size_t i;
    for (i = 1; i < fullPages; i++){
        readFrame(ram, getFrame(PID, firstPage + i), aux);
        aux += toRead;
    }

    // Leer "pedacito" al inicio de ultima pagina.
    toRead = bytes - toRead;
    memcpy(aux, ram_getFrame(ram, getFrame(PID, firstPage + i)), toRead);
    
}
// Unused, reemplazado por heapWrite()
void memwrite(uint32_t bytes, uint32_t address, int PID, void *from){
    uint32_t firstPage = getPage(address, config);
    uint32_t offset = getOffset(address, config);

    uint32_t toWrite = min(bytes, config->pageSize - offset);
    uint32_t fullPages = (bytes - toWrite) / config->pageSize;

    uint32_t firstFrame = getFrame(PID, firstPage);
    void *aux = from;

    // Escribir "pedacito" de memoria en el final de una pag.
    memcpy(ram_getFrame(ram, firstFrame), aux, toWrite);
    aux += toWrite;

    // Escribir frames del medio completos.
    toWrite = config->pageSize;
    size_t i;
    for (i = 1; i < fullPages; i++){
        writeFrame(ram, getFrame(PID, firstPage + 1), aux);
        aux += toWrite;
    }

    // Escribir "pedacito" al inicio de ultima pagina.
    toWrite = bytes - toWrite;
    memcpy(ram_getFrame(ram, getFrame(PID, firstPage + i)), aux, toWrite);
}
// UNUSED.
void createPages(uint32_t PID, uint32_t qty){
    t_pageTable* pt = getPageTable(PID, pageTables);
    
    // No existe funcion para crear pag vacia en swap.

    for (uint32_t i = 0; i < qty; i++){
        uint32_t pageN = pageTableAddEntry(pt, -1);
        if (! swapInterface_createEmptyPage(swapInterface, PID, pageN)){
            pthread_mutex_lock(&logMut);
            log_error(memLogger, "No se pudo crear pagina Nro. %i en swap para PID: %i", pageN, PID);
            pthread_mutex_unlock(&logMut);
        }
    }
}
*/