#include "process.h"

t_process* createProcess(uint32_t id, int socket, double estimator){
    t_process* process = malloc(sizeof(t_process));
    process->pid = id;
    process->socket = socket;
    process->state = NEW;
    process->estimator = estimator;
    process->waited = 0;
    return process;
}

void destroyProcess(t_process* process){
    free(process);
}