#include <stdint.h>
#include <commons/string.h>
#include "memory.h"
#include "networking.h"
#include "commons/log.h"
#include "commons/config.h"
#include "swapInterface.h"
#include "utils.h"

// TODO: Preguntar sobre coordinacion de estructuras compartidas:
// ram, metaData, pageTables.


int main(){

    // Initialize logger.
    logger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
    
    // Load and validate config
    config = getMemoryConfig("./memory.cfg");
    validateConfg(config, logger);
    
    ram = initializeMemory(config);
    metadata = initializeMemoryMetadata(config);
    pageTables = dictionary_create();

    // swapInterface = swapInterface_create(config->swapIP, config->swapPort, config->pageSize, /*ALGO*/);  // TODO Add algoritmo

    int serverSocket = createListenServer(config->ip, config->port);
    
    while(1){
        runListenServer(serverSocket, petitionHandler);
    }

    return 0;
}

void *petitionHandler(void *_clientSocket){
    // TODO: Chequear logica petitionHandler. 
    uint32_t clientSocket = (int*) _clientSocket;
    t_packet *petition = socket_getPacket(clientSocket);
    petitionHandlers[petition->header](petition, clientSocket);
}

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config){
    t_memoryMetadata *newMetadata = malloc(sizeof(t_memoryMetadata));
    newMetadata->entryQty = config->frameQty;
    newMetadata->entries = calloc(newMetadata->entryQty, sizeof(t_frameMetadata));

    for (int i = 0; i < newMetadata->entryQty; i++){
        ((newMetadata->entries)[i])->isFree = 1;
    }

    return newMetadata; 
}

void destroyMemoryMetadata(t_memoryMetadata *metadata){
    free(metadata->entries);
    free(metadata);
}

t_memory *initializeMemory(t_memoryConfig *config){
    t_memory *newMemory = malloc(sizeof(t_memory));
    newMemory->memory = calloc(1,config->size);
    newMemory->config = config;

    return newMemory;
}

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

int32_t getFreeFrame(uint32_t start, uint32_t end){
    for(uint32_t i = start; i < end; i++){
        if(isFree(i)) return i;
    }
    return -1;
}

void readFrame(t_memory *mem, uint32_t frame, void *dest){
    void *frameAddress = ram_getFrame(mem, frame);
    memcpy(dest, frameAddress, mem->config->pageSize);
}

void writeFrame(t_memory *mem, uint32_t frame, void *from){
    void *frameAddress = ram_getFrame(mem, frame);
    memcpy(frameAddress, from, mem->config->pageSize);
}

void *ram_getFrame(t_memory *mem, uint32_t frame){
    return mem->memory + frame * config->pageSize;
}

int32_t getPage(uint32_t address, t_memoryConfig *cfg){
    return address / cfg->pageSize;
}

int32_t getOffset(uint32_t address, t_memoryConfig *cfg){
    return address % cfg->pageSize;
}

void fijo(int32_t *start, int32_t *end, uint32_t PID){
    *start = getFrame(PID, 0);
    *end = *start + config->framesPerProcess;
}

void global(int32_t *start, int32_t *end, uint32_t PID){
    *start = 0;
    *end = config->frameQty;
}

int32_t getFrame(uint32_t PID, uint32_t pageN){
    t_pageTable* pt = getPageTable(PID, pageTables);

    /* TODO: Integrar con TLB:
        * Buscar en TLB antes de tabla de paginas.
        * Actualizar TLB cuando se swapean paginas.
     */

    // Si tiene menos paginas de las que se piden, hay que crear el resto.
    if (pageN > pt->pageQuantity - 1){
        createPages(PID, pageN - pt->pageQuantity + 1);
        return getFrame(PID, pageN);
    }

    // Si esta presente retorna el numero de frame.
    if (((pt->entries)[pageN]).present) return ((pt->entries)[pageN]).frame;

    // Si no esta presente hay que traerla de swap.
    return swapPage(PID, pageN);

}

bool isPresent(uint32_t PID, uint32_t page){
    t_pageTable* pt = getPageTable(PID, pageTables);
    return (pt->entries)[page].present;
}

void createPages(uint32_t PID, uint32_t qty){
    t_pageTable* pt = getPageTable(PID, pageTables);
    
    // TODO: Crear pag vacia en swap.

    for (uint32_t i = 0; i < qty; i++){
        uint32_t pageN = pageTableAddEntry(pt, -1);
        if (! swapInterface_createEmptyPage(swapInterface, PID, pageN)){
            pthread_mutex_lock(&mutex_log);
            log_error(logger, "No se pudo crear pagina Nro. %i en swap para PID: %i", pageN, PID);
            pthread_mutex_unlock(&mutex_log);
        }
    }
}

uint32_t swapPage(uint32_t PID, uint32_t page){

    uint32_t start, end;
    assignacion(&start, &end, PID);

    uint32_t victima = algoritmo(start, end);

    return replace(victima, PID, page);

}

uint32_t replace(uint32_t victim, uint32_t PID, uint32_t page){
    // Traer pagina pedida de swap.
    void *pageFromSwap = swapInterface_loadPage(swapInterface, PID, page);

    // Chequear que se haya podido traer.
    if (!pageFromSwap){
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "No se pudeo cargar pagina #%i del PID #%i", page, PID);
        pthread_mutex_unlock(&mutex_log);        
        return -1;
    }

    // Si el frame no esta libre se envia a swap la pagina que lo ocupa. 
    // Esto es para que replace() se pueda utilizar tanto para cargar paginas a frames libres como para reemplazar.
    if (! isFree(victim)){
        // Enviar pagina reemplazada a swap.
        uint32_t victimPID  = (metadata->entries)[victim]->PID;
        uint32_t victimPage = (metadata->entries)[victim]->page;
        swapInterface_savePage(swapInterface, victimPID, victimPage, ram_getFrame(ram, victim));
        // Modificar tabla de paginas del proceso cuya pagina fue reemplazada.
        t_pageTable *ptReemplazado = getPageTable(victimPID, pageTables);
        (ptReemplazado->entries)[victimPage].present = false;
        (ptReemplazado->entries)[victimPage].frame = -1;
    }

    // Escribir pagina traida de swap a memoria.
    writeFrame(ram, victim, pageFromSwap);
    // Modificar tabla de paginas del proceso cuya pagina entra a memoria.
    t_pageTable *ptReemplaza = getPageTable(PID, pageTables);
    (ptReemplaza->entries)[page].present = true;
    (ptReemplaza->entries)[page].frame = victim;

    // Modificar frame metadata.
    (metadata->entries)[victim]->page = page;
    (metadata->entries)[victim]->PID = PID;
    (metadata->entries)[victim]->isFree = false;

    return victim;
}

bool isFree(uint32_t frame){
    return (metadata->entries)[frame]->isFree;
}
uint32_t getFrameTimestamp(uint32_t frame){
    return (metadata->entries)[frame]->timeStamp;
}

uint32_t LRU(uint32_t start, uint32_t end){
    
    int32_t frame = getFreeFrame(start, end);
    if(frame != -1) return frame;

    int32_t min;
    bool firstIter = 0;

    for(uint32_t i = start; i < end; i++){
        if (firstIter || getFrameTimestamp(i) < min){
            frame = i;
            min = getFrameTimestamp(i);
            firstIter = false;
        }
    }

    return frame;
}

void ram_editFrame(t_memory *mem, uint32_t offset, uint32_t frame, void *from, uint32_t size){
    void *frameAddress = ram_getFrame(mem, frame);
    void *dest = frameAddress + offset;

    memcpy(dest, from, size);
}