#ifndef PROCESS_H_
#define PROCESS_H_
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

typedef enum state { NEW, READY, BLOCKED, EXEC, SUSP_READY, SUSP_BLOCKED, TERMINATED } t_state;

typedef struct process{
    uint32_t pid;
    int socket;
    t_state state;
    double estimator;
    int waited;
    struct timespec startTime;
} t_process;

/**
 * @DESC: Crea un objeto Process en memoria
 * @param id: id asignado al proceso
 * @param socket: socket con el que se conecto el proceso
 * @return t_process*: puntero al Proceso
 */
t_process* createProcess(uint32_t id, int socket, double estimator);

/**
 * @DESC: Destruye un Proceso de memoria
 * @param process: puntero al proceso a destruir
 */
void destroyProcess(t_process* process);

#endif // !PROCESS_H_