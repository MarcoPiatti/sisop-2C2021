/**
 * @file: petitionHandlers.c
 * @author pepinOS 
 * @DESC: Funciones que ejecutan los CPUs para atender pedidos concretos.
 * Este .c incluye directamente a kernel.h y no a su propio header,
 * debido a que interactua con variables globales (colas de estado, por ejemplo).
 * Podria tranquilamente agregarse todo al .c principal, pero separado queda mas entendible. 
 * 
 * @version 0.1
 * @date: 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "kernel.h"

// Si un proceso pidio inicializar un semaforo, se ejecuta esta funcion
bool semInit(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    t_mateSem* sem = NULL;
    t_packet* ddTell = NULL;
    char* semName = streamTake_STRING(petition->payload);
    int32_t semValue = streamTake_INT32(petition->payload);
    pthread_mutex_lock(&mutex_sem_dict);
    if(!dictionary_has_key(sem_dict, semName)){
        sem = mateSem_create(semName, (unsigned int)semValue, thread_semFunc);
        dictionary_put(sem_dict, semName, sem);
        pthread_mutex_unlock(&mutex_sem_dict);
        response = createPacket(OK, 0);

        ddTell = createPacket(DD_SEM_INIT, INITIAL_STREAM_SIZE);
        streamAdd_UINT32(ddTell->payload, (uint32_t)sem);
        streamAdd_INT32(ddTell->payload, semValue);
        pQueue_put(dd->queue, (void*)ddTell);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: crea el semaforo %s", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);
    }
    else{
        pthread_mutex_unlock(&mutex_sem_dict);
        
        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Proceso %i: trata de crear el semaforo %s, que ya existe", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);

        response = createPacket(ERROR, 0);
    }
    socket_sendPacket(process->socket, response);
    destroyPacket(response);
    free(semName);
    return true;
}

// Si un proceso pidio un wait de un semaforo, se ejecuta esta
bool semWait(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    char* semName = streamTake_STRING(petition->payload);
    t_mateSem* sem = NULL;
    t_packet *ddTell = NULL;
    bool rc;
    pthread_mutex_lock(&mutex_sem_dict);
    if(dictionary_has_key(sem_dict, semName)){
        sem = (t_mateSem*)dictionary_get(sem_dict, semName);
        pthread_mutex_unlock(&mutex_sem_dict);

        
        if(mateSem_wait(sem, process)){
            pthread_mutex_lock(&mutex_mediumTerm);
                process->state = BLOCKED;
                pQueue_put(blockedQueue, process);
            pthread_cond_signal(&cond_mediumTerm);
            pthread_mutex_unlock(&mutex_mediumTerm);
            
            ddTell = createPacket(DD_SEM_REQ, INITIAL_STREAM_SIZE);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "Proceso %i: se frena en el semaforo %s", process->pid, semName);
            pthread_mutex_unlock(&mutex_log);

            rc = false;
        }
        else {

            ddTell = createPacket(DD_SEM_ALLOC_INST, INITIAL_STREAM_SIZE);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "Proceso %i: descuenta el semaforo %s", process->pid, semName);
            pthread_mutex_unlock(&mutex_log);

            response = createPacket(OK, 0);
            socket_sendPacket(process->socket, response);
            destroyPacket(response);

            rc = true;
        }

        streamAdd_UINT32(ddTell->payload,(uint32_t)process);
        streamAdd_UINT32(ddTell->payload,(uint32_t)sem);
        pQueue_put(dd->queue, (void*)ddTell);
    }
    else{
        pthread_mutex_unlock(&mutex_sem_dict);

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Proceso %i: trata de esperar en el semaforo %s, que no existe", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);

        response = createPacket(ERROR, 0);
        socket_sendPacket(process->socket, response);
        destroyPacket(response);
        rc = true;
    }
    free(semName);
    return rc;
}

// Si un proceso hizo post de un semaforo, se ejecuta esta
bool semPost(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    char* semName = streamTake_STRING(petition->payload);
    t_mateSem* sem = NULL;
    t_packet *ddTell = NULL;
    pthread_mutex_lock(&mutex_sem_dict);
    if(dictionary_has_key(sem_dict, semName)){
        sem = (t_mateSem*)dictionary_get(sem_dict, semName);
        pthread_mutex_unlock(&mutex_sem_dict);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: postea el semaforo %s", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);
        
        mateSem_post(sem);

        ddTell = createPacket(DD_SEM_REL, INITIAL_STREAM_SIZE);
        streamAdd_UINT32(ddTell->payload,(uint32_t)process);
        streamAdd_UINT32(ddTell->payload,(uint32_t)sem);
        pQueue_put(dd->queue, (void*)ddTell);

        response = createPacket(OK, 0);
    }
    else {
        pthread_mutex_unlock(&mutex_sem_dict);
        
        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Proceso %i: trata de postear el semaforo %s, que no existe", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);

        response = createPacket(ERROR, 0);
    }
    
    socket_sendPacket(process->socket, response);
    destroyPacket(response);
    free(semName);
    return true;
}

// Si un proceso pidio destruir un semaforo, se ejecuta esta
bool semDestroy(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    t_packet *ddTell = NULL;
    char* semName = streamTake_STRING(petition->payload);
    pthread_mutex_lock(&mutex_sem_dict);
    if(dictionary_has_key(sem_dict, semName)){
        void destroyer(void*elem){
            mateSem_destroy((t_mateSem*)elem);
        };
        t_mateSem* sem = dictionary_get(sem_dict, semName);
        ddTell = createPacket(DD_SEM_DESTROY, INITIAL_STREAM_SIZE);
        streamAdd_UINT32(ddTell->payload,(uint32_t)sem);
        pQueue_put(dd->queue, (void*)ddTell);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: destruye el semaforo %s", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);

        dictionary_remove_and_destroy(sem_dict, semName, destroyer);
        response = createPacket(OK, 0);
    }
    else{

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Proceso %i: trata de destruir el semaforo %s, que no existe", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);

        response = createPacket(ERROR, 0);
    }
    pthread_mutex_unlock(&mutex_sem_dict);
    socket_sendPacket(process->socket, response);
    destroyPacket(response);
    free(semName);
    return true;
}

// Si un proceso pidio esperar a un IO, se ejecuta esta
bool callIO(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    char* IOName = streamTake_STRING(petition->payload);
    t_IODevice* device = NULL;
    pthread_mutex_lock(&mutex_IO_dict);
    if(dictionary_has_key(IO_dict, IOName)){
        device = (t_IODevice*)dictionary_get(IO_dict, IOName);
        pthread_mutex_unlock(&mutex_IO_dict);

        pthread_mutex_lock(&mutex_mediumTerm);
            process->state = BLOCKED;
            pQueue_put(blockedQueue, process);
        pthread_cond_signal(&cond_mediumTerm);
        pthread_mutex_unlock(&mutex_mediumTerm);
        
        pQueue_put(device->waitingProcesses, process);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: llama al dispositivo IO %s", process->pid, IOName);
        pthread_mutex_unlock(&mutex_log);

        free(IOName);
        return false;
    }
    else{
        pthread_mutex_unlock(&mutex_IO_dict);

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Proceso %i: llama al dispositivo IO %s, que no existe", process->pid, IOName);
        pthread_mutex_unlock(&mutex_log);

        response = createPacket(ERROR, 0);
        socket_sendPacket(process->socket, response);
        destroyPacket(response);
        free(IOName);
        return true;
    }
    
}

// Si un proceso pidio algo de memoria, se ejecuta esta
bool relayPetition(t_process* process, t_packet* petition, int memorySocket){
    socket_relayPacket(memorySocket, petition);
    t_packet* response = socket_getPacket(memorySocket);
    
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: hace un pedido a memoria", process->pid);
    pthread_mutex_unlock(&mutex_log);
    
    socket_relayPacket(process->socket, response);
    destroyPacket(response);
    return true;
}

// Si un proceso nos dijo chau viejo me termino, se ejecuta La de abajo, que llama a esta
bool terminateProcess(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = createPacket(OK, 0);
    socket_sendPacket(process->socket, response);

    t_packet* ddTell = createPacket(DD_PROC_TERM, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(ddTell->payload, (uint32_t)process);
    pQueue_put(dd->queue, (void*)ddTell);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %i: se desconecta", process->pid);
    pthread_mutex_unlock(&mutex_log);
    
    destroyPacket(response);
    destroyProcess(process);
    sem_post(&sem_multiprogram);
    return false;
}

// Si un proceso nos dijo chau viejo me termino, se ejecuta esta, que avisa a la memoria del evento
bool relayTerminate(t_process* process, t_packet* petition, int memorySocket){
    socket_relayPacket(memorySocket, petition);
    t_packet* response = socket_getPacket(memorySocket);
    socket_relayPacket(process->socket, response);
    destroyPacket(response);

    t_packet* finalPacket = socket_getPacket(process->socket);
    terminateProcess(process, finalPacket, memorySocket);
    destroyPacket(finalPacket);
    return false;
}

bool(*petitionHandlers[MAX_PETITIONS])(t_process* process, t_packet* petition, int memorySocket) = 
{   
    NULL,
    semInit,
    semWait,
    semPost,
    semDestroy,
    callIO,
    relayPetition,
    relayPetition,
    relayPetition,
    relayPetition,
    relayTerminate,
    terminateProcess
};