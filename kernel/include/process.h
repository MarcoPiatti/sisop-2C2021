#ifndef PROCESS_H_
#define PROCESS_H_

typedef enum state { NEW, READY, BLOCKED, EXEC, SUSP_READY, SUSP_BLOCKED, TERMINATED } t_state;

typedef struct process{
    int pid;
    int socket;
    t_state state;
} t_process;

/**
 * @DESC: Crea un objeto Process en memoria
 * @param id: id asignado al proceso
 * @param socket: socket con el que se conecto el proceso
 * @return t_process*: puntero al Proceso
 */
t_process* createProcess(int id, int socket);

/**
 * @DESC: Destruye un Proceso de memoria
 * @param process: puntero al proceso a destruir
 */
void destroyProcess(t_process* process);

#endif // !PROCESS_H_