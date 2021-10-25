#include "swap.h"


void main(void){
    swapLogger = log_create("./swap.log", "SWAP", 1, LOG_LEVEL_TRACE);

    t_swapConfig *swapConfig = getswapConfig("./swap.cfg");

    t_list *swapFiles = list_create();
    for(int i = 0; swapConfig->swapFiles[i]; i++){
        list_add(swapFiles, swapFile_create(swapConfig->swapFiles[i],
        swapConfig->fileSize,
        swapConfig->pageSize));
    } 
    
    int serverSocket = createListenServer(swapConfig->swapIP, swapConfig->swapPort);
    int clientSocket = getNewClient(serverSocket);

    int header = socket_getHeader(clientSocket);

    if (header == ASIG_FIJA){
        asignacion = fija;
    }
    if (header == ASIG_GLOBAL){
        asignacion = global;
    }

    t_packet *receivedPetition;

    while (1){
        receivedPetition = socket_getPacket(clientSocket);
        milliSleep(swapConfig->delay);
        petitionHandler[receivedPetition->header](receivedPetition, clientSocket);
        destroyPacket(receivedPetition);
    }
    
}

int fija(int PID, int PAGE, void *content){
    return true;
}

int global(int PID, int page, void *content){
    return true;
}