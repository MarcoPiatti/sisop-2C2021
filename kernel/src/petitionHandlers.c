#include "kernel.h"

bool semInit(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response;
    char* semName = streamTake_STRING(petition->payload);
    int32_t semValue = streamTake_INT32(petition->payload);

    if(!dictionary_has_key(sem_dict, semName)){
        dictionary_put(sem_dict, semName, mateSem_create(semName, (unsigned int)semValue, thread_semFunc));
        response = createPacket(OK, 0);
    }
    else response = createPacket(ERROR, 0);
    
    socket_sendPacket(process->socket, response);
    destroyPacket(response);
    free(semName);
    return true;
}

bool semWait(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response;
    char* semName = streamTake_STRING(petition->payload);
    t_mateSem* sem;
    if(dictionary_has_key(sem_dict, semName)){
        sem = (t_mateSem*)dictionary_get(sem_dict, semName);
        pQueue_put(blockedQueue, process);
        mateSem_wait(sem, process);
        free(semName);
        return false;
    }
    else{
        response = createPacket(ERROR, 0);
        socket_sendPacket(process->socket, response);
        destroyPacket(response);
        free(semName);
        return true;
    }
    
}

bool semPost(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response;
    char* semName = streamTake_STRING(petition->payload);
    t_mateSem* sem;
    if(dictionary_has_key(sem_dict, semName)){
        sem = (t_mateSem*)dictionary_get(sem_dict, semName);
        mateSem_post(sem);
        response = createPacket(OK, 0);
    }
    else response = createPacket(ERROR, 0);
    
    socket_sendPacket(process->socket, response);
    destroyPacket(response);
    free(semName);
    return true;
}

bool semDestroy(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response;
    char* semName = streamTake_STRING(petition->payload);
    if(dictionary_has_key(sem_dict, semName)){
        void destroyer(void*elem){
            mateSem_destroy((t_mateSem*)elem);
        }
        dictionary_remove_and_destroy(sem_dict, semName, destroyer);
        response = createPacket(OK, 0);
    }
    else{
        response = createPacket(ERROR, 0);
    }
    
    socket_sendPacket(process->socket, response);
    destroyPacket(response);
    free(semName);
    return true;
}

bool callIO(t_process* process, t_packet* petition, int memorySocket){
    t_packet *response;
    char* IOName = streamTake_STRING(petition->payload);
    t_IODevice* device;
    if(dictionary_has_key(IO_dict, IOName)){
        device = (t_IODevice*)dictionary_get(IO_dict, IOName);
        pQueue_put(blockedQueue, process);
        pQueue_put(device->waitingProcesses, process);
        free(IOName);
        return false;
    }
    else{
        response = createPacket(ERROR, 0);
        socket_sendPacket(process->socket, response);
        destroyPacket(response);
        free(IOName);
        return true;
    }
    
}

bool relayPetition(t_process* process, t_packet* petition, int memorySocket){
    socket_relayPacket(memorySocket, petition);
    t_packet* response = socket_getPacket(memorySocket);
    socket_relayPacket(process->socket, response);
    destroyPacket(response);
    return true;
}

bool terminateProcess(t_process* process, t_packet* petition, int memorySocket){
    destroyProcess(process);
    return false;
}

const bool(*petitionHandlers[MAX_PETITIONS])
    (t_process* process, t_packet* petition, int memorySocket) = 
{
    semInit,
    semWait,
    semPost,
    semDestroy,
    callIO,
    relayPetition,
    relayPetition,
    relayPetition,
    relayPetition,
    terminateProcess
}