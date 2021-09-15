/**
 * @file: process.h
 * @author pepinOS 
 * @DESC: TAD para un proceso en el kernel
 * @version 0.1
 * @date: 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#include <time.h>

typedef enum state { NEW, READY, BLOCKED, EXEC, SUSP_READY, SUSP_BLOCKED, TERMINATED } t_state;

typedef struct process{
    int pid;
    int socket;
    t_state state;
    double estimate;
    time_t waitedTime;
} t_process;

/**
 * @DESC: Crea un objeto Process en memoria
 * @param id: id asignado al proceso
 * @param socket: socket con el que se conecto el proceso
 * @param estimate: estimador inicial
 * @return t_process*: puntero al Proceso
 */
t_process* createProcess(int id, int socket, double estimate);

/**
 * @DESC: Destruye un Proceso de memoria
 * @param process: puntero al proceso a destruir
 */
void destroyProcess(t_process* process);

#endif // !PROCESS_H_