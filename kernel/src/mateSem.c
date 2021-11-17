#include "mateSem.h"

#include "commons/string.h"
#include <stdlib.h>

t_mateSem* mateSem_create(char* nombre, unsigned int contadorInicial){
    t_mateSem* mateSem = malloc(sizeof(t_mateSem));
    mateSem->nombre = string_duplicate(nombre);
    mateSem->semaforo = contadorInicial;
    mateSem->waitingProcesses = pQueue_create();
    return mateSem;
}

void mateSem_destroy(t_mateSem* mateSem){
    void _destroyProcess(void *process){
        destroyProcess((t_process*) process);
    }
    pQueue_destroy(mateSem->waitingProcesses, _destroyProcess);
    free(mateSem->nombre);
    free(mateSem);
}

processState mateSem_wait(t_mateSem* mateSem, t_process* process, t_processQueues direcciones){
    mateSem->semaforo = mateSem->semaforo - 1;
    if(mateSem->semaforo < 0){
        pQueue_put(mateSem->waitingProcesses, (void*)process);
        pQueue_put(direcciones.blockedQueue, process);
        return BLOCKED;
    } else {
        return CONTINUE;
    }
}

void mateSem_post(t_mateSem* mateSem, t_processQueues direcciones){
    //Incremento semáforo
    mateSem->semaforo = mateSem->semaforo + 1;
    
    //Si hay proceso esperando...
    if(mateSem->semaforo <= 0){
        t_process* process = pQueue_take(mateSem->waitingProcesses);

        bool getProcessById(void* elem) {
            return ((t_process*)elem)->pid == process->pid;
        }

        //Blocked -> Ready
        if(process->state==BLOCKED){
            pQueue_removeBy(direcciones.blockedQueue, getProcessById);
            pQueue_put(direcciones.readyQueue, process);
        } 
        //Suspended Blocked -> Ready
        else {
            pQueue_removeBy(direcciones.suspendedBlockedQueue, getProcessById);
            pQueue_put(direcciones.suspendedReadyQueue, process);
        }
    }
}


//TODO: relevar y cambiar según corresponda