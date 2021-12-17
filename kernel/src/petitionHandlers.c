#include "kernel.h"

bool semInit(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    t_mateSem* sem = NULL;
    char* semName = streamTake_STRING(petition->payload);
    int32_t semValue = streamTake_INT32(petition->payload);
    pthread_mutex_lock(&mutex_sem_dict);
    if(!dictionary_has_key(sem_dict, semName)){
        sem = mateSem_create(semName, (unsigned int)semValue, thread_semFunc);
        dictionary_put(sem_dict, semName, sem);
        pthread_mutex_unlock(&mutex_sem_dict);
        response = createPacket(OK, 0);

        DDSemInit(dd, sem, semValue);

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

bool semWait(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    char* semName = streamTake_STRING(petition->payload);
    t_mateSem* sem = NULL;
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
            
            DDSemRequested(dd, process, sem);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "Proceso %i: se frena en el semaforo %s", process->pid, semName);
            pthread_mutex_unlock(&mutex_log);

            rc = false;
        }
        else {
            DDSemAllocatedInstant(dd, process, sem);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "Proceso %i: descuenta el semaforo %s", process->pid, semName);
            pthread_mutex_unlock(&mutex_log);

            response = createPacket(OK, 0);
            socket_sendPacket(process->socket, response);
            destroyPacket(response);

            rc = true;
        }
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


bool semPost(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    char* semName = streamTake_STRING(petition->payload);
    t_mateSem* sem = NULL;
    pthread_mutex_lock(&mutex_sem_dict);
    if(dictionary_has_key(sem_dict, semName)){
        sem = (t_mateSem*)dictionary_get(sem_dict, semName);
        pthread_mutex_unlock(&mutex_sem_dict);

        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: postea el semaforo %s", process->pid, semName);
        pthread_mutex_unlock(&mutex_log);
        
        mateSem_post(sem);

        DDSemRelease(dd, process, sem);

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


bool semDestroy(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = NULL;
    char* semName = streamTake_STRING(petition->payload);
    pthread_mutex_lock(&mutex_sem_dict);
    if(dictionary_has_key(sem_dict, semName)){
        void destroyer(void*elem){
            mateSem_destroy((t_mateSem*)elem);
        };
        t_mateSem* sem = dictionary_get(sem_dict, semName);

        DDSemDestroy(dd, sem);

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
        
        pthread_mutex_lock(&mutex_log);
        log_info(logger, "Proceso %i: se agrega a la cola de espera del dispositivo IO %s", process->pid, IOName);
        pthread_mutex_unlock(&mutex_log);
        
        pQueue_put(device->waitingProcesses, process);

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

// memalloc
bool relayPetition(t_process* process, t_packet* petition, int memorySocket){
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %u: hace un pedido a memoria", process->pid);
    pthread_mutex_unlock(&mutex_log);
    
    socket_relayPacket(memorySocket, petition);
    t_packet* response = socket_getPacket(memorySocket);
    
    socket_relayPacket(process->socket, response);
    destroyPacket(response);
    return true;
}

// terminated
bool terminateProcess(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response = createPacket(OK, 0);
    socket_sendPacket(process->socket, response);

    DDProcTerm(dd, process);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Proceso %i: se desconecta", process->pid);
    pthread_mutex_unlock(&mutex_log);
    
    destroyPacket(response);
    destroyProcess(process);
    sem_post(&sem_multiprogram);
    pthread_mutex_lock(&mutex_cupos);
        cuposLibres++;
    pthread_mutex_unlock(&mutex_cupos);
    return false;
}

// terminated (a memoria)
bool relayTerminate(t_process* process, t_packet* petition, int memorySocket){
    socket_relayPacket(memorySocket, petition);
    t_packet* response = socket_getPacket(memorySocket);
    socket_relayPacket(process->socket, response);
    destroyPacket(response);
    return true;
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
    NULL,
    relayTerminate,
    terminateProcess
};


/*
    CAPI_ID,            // | HEADER | PAYLOAD_SIZE | PID = UINT32 |
    SEM_INIT,           // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING | SEM_VALUE = UINT32 |
    SEM_WAIT,           // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING |
    SEM_POST,           // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING |
    SEM_DESTROY,        // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING |
    CALL_IO,            // | HEADER | PAYLOAD_SIZE | IO_NAME = STRING  |
    MEMALLOC,           // | HEADER | PAYLOAD_SIZE | PID = UINT32 | SIZE = INT32 |
    MEMFREE,            // | HEADER | PAYLOAD_SIZE | PID = UINT32 | PTR = INT32 |
    MEMREAD,            // | HEADER | PAYLOAD_SIZE | PID = UINT32 | PTR = INT32 | SIZE = INT32 |
    MEMWRITE,           // | HEADER | PAYLOAD_SIZE | PID = UINT32 | PTR = INT32 | DATASIZE = INT32 | DATA = STREAM |
    CAPI_TERM,          // | HEADER | PAYLOAD_SIZE | PID = UINT32 |
    DISCONNECTED,       // | HEADER | PAYLOAD_SIZE = 0 |
*/