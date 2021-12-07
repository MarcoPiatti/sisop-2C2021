#include <kernel.h>

processState _capi_id(t_packet *received, t_process* process){

    return CONTINUE;
}

processState _sem_init(t_packet *received, t_process* process){


    char* semName = streamTake_STRING(received->payload);
    uint32_t semCount = streamTake_UINT32(received->payload);
    t_mateSem* newSem = mateSem_create(semName, semCount);
    dictionary_put(mateSems, semName, newSem);
    
    pthread_mutex_lock(&mutex_log);
    log_info(kernelLogger, "El proceso %u inicializo un semaforo <%s> con valor %d", process->pid, semName, semCount);
    pthread_mutex_unlock(&mutex_log);

    DDSemInit(deadlockDetector, newSem, semCount);

    return CONTINUE;
}

processState _sem_wait(t_packet *received, t_process* process){
    char* nombreSem = streamTake_STRING(received->payload);
    t_mateSem* semaforo = dictionary_get(mateSems, nombreSem); //fetch semaphore from dictionary

    pthread_mutex_lock(&mutex_log);
    log_info(kernelLogger, "El proceso %u realizo un wait al semaforo <%s>", process->pid, semaforo->nombre);
    pthread_mutex_unlock(&mutex_log);

    return mateSem_wait(semaforo, process, processQueues);
}

processState _sem_post(t_packet *received, t_process* process){
    char* nombreSem = streamTake_STRING(received->payload);
    t_mateSem* semaforo = dictionary_get(mateSems, nombreSem);
    mateSem_post(semaforo, processQueues);

    DDSemRelease(deadlockDetector, process, semaforo);

    pthread_mutex_lock(&mutex_log);
    log_info(kernelLogger, "El proceso %u realizo un post al semaforo <%s>", process->pid, semaforo->nombre);
    pthread_mutex_unlock(&mutex_log);

    return CONTINUE;
}

processState _sem_destroy(t_packet *received, t_process* process){
    char* nombreSem = streamTake_STRING(received->payload);
    t_mateSem* semaforo = dictionary_get(mateSems, nombreSem);

    pthread_mutex_lock(&mutex_log);
    log_info(kernelLogger, "El proceso %u esta destruyendo al semaforo <%s>", process->pid, semaforo->nombre);
    pthread_mutex_unlock(&mutex_log);
    
    DDSemDestroy(deadlockDetector, semaforo);

    mateSem_destroy(semaforo);
    dictionary_remove(mateSems, nombreSem);


    return CONTINUE;
}

processState _call_io(t_packet *received, t_process* process){
    /* todo tuyo rey
    pthread_mutex_lock(&mutex_log);
    log_info(kernelLogger, "", process->pid);
    pthread_mutex_unlock(&mutex_log);
    */

    return BLOCK;    //TODO: Ulises
}

processState _capi_term(t_packet *received, t_process* process){
    //Avisar a memoria. Implica recibir un disconnected inmediatamente despues
    DDProcTerm(deadlockDetector, process);
    return CONTINUE;
}

processState _disconnected(t_packet *received, t_process* process){
    //Pasar a exit
    return EXIT;
}

void relayPetition(t_packet* packet, uint32_t socket) {
    socket_relayPacket(memSocket, packet);
    t_packet* response = socket_getPacket(memSocket);
    
    socket_relayPacket(socket, response);
    destroyPacket(response);
}

processState (*petitionProcessHandler[MAX_PETITIONS])(t_packet *received, t_process* process) = {
    _capi_id,
    _sem_init,
    _sem_wait,
    _sem_post,
    _sem_destroy,
    _call_io,
    relayPetition,
    relayPetition,
    relayPetition,
    relayPetition,
    NULL,
    _capi_term,
    _disconnected
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