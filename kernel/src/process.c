#include "process.h"

t_process* createProcess(int id, int socket){
    t_process* process = malloc(sizeof(t_process));
    process->pid = id;
    process->socket = socket;
    process->state = NEW;
    return process;
}

void destroyProcess(t_process* process){
    free(process);
}