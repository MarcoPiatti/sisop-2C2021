#include "memory.h"

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
    logger = log_create("./cfg/kernel.log", "Memoria", true, LOG_LEVEL_TRACE);
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