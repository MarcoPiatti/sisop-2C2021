#include "memory.h"

void* thread_clientHandlerFunc(void* args){
    int clientSocket = (int)args;
    socket_sendHeader(clientSocket, ID_MEMORIA);
    t_packet* petition;
    bool keepServing = true;
    while(keepServing){
        petition = socket_getPacket(clientSocket);
        keepServing = petitionHandlers[petition->header](clientSocket);
        destroyPacket(petition);
    }
    
    return 0;
}

int main(){
    memoryConfig = getMemoryConfig("./cfg/memory.config");

    swapHeader asign;
    if(strcmp(memoryConfig->tipoAsignacion, "FIJA")){
        asign = ASIGN_FIJO;
    }
    else if(strcmp(memoryConfig->tipoAsignacion, "GLOBAL")){
        asign = ASIGN_GLOBAL;
    }
    ram = malloc(memoryConfig->ramSize);

    if(strcmp(memoryConfig->algoritmoTLB, "FIFO")){
        tlb = createTLB(memoryConfig->TLBSize, fifo);
    }
    else if(strcmp(memoryConfig->algoritmoTLB, "LRU")){
        tlb = createTLB(memoryConfig->TLBSize, lru);
    }
    

    swapInterface = swapInterface_create(memoryConfig->swapIp
                                        ,memoryConfig->swapPort
                                        ,memoryConfig->pageSize
                                        ,asign);
    
    int memoryServer = createListenServer(memoryConfig->memoryIp, memoryConfig->memoryPort);
    
    while(1){
        runListenServer(memoryServer, thread_clientHandlerFunc);
    }
    return 0;
}