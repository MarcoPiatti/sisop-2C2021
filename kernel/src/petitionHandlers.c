#include <kernel.h>

bool _sem_wait(t_packet *received, int clientSocket){
    return true;
}

bool _call_io(t_packet *received, int clientSocket){
    return true;
}

bool (*petitionProcessHandler[MAX_PETITIONS])(t_packet *received, int clientSocket) = {
    _sem_wait,
    _call_io
};
