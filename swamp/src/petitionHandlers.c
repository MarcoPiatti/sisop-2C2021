#include "swap.h"

void savePage(t_packet *received, int clientSocket){
    ;
}

void readPage(t_packet *received, int clientSocket){
    ;
}

void destroyPage(t_packet *received, int clientSocket){
    ;
}

void memDisconnect(t_packet *received, int clientSocket){
    ;
}

void (*petitionHandler[MAX_MEM_PETITIONS])(t_packet *received, int clientSocket) = {
    savePage,
    readPage,
    destroyPage,
    memDisconnect
};
