#include "matelib.h"

#include "networking.h"
#include <commons/config.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <commons/log.h>
#include <commons/string.h>

typedef struct mate_inner_structure{
    uint32_t pid;
    t_config* mateConfig;
    char* mateIP;
    char* matePort;
    int mateSocket;
    bool isMemory;
    t_log* logger;
} mate_inner_structure;

//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config){
    lib_ref->group_info = malloc(sizeof(mate_inner_structure));
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    mateStruct->mateConfig = config_create(config);
    mateStruct->mateIP = config_get_string_value(mateStruct->mateConfig, "IP_MATE");
    mateStruct->matePort = config_get_string_value(mateStruct->mateConfig, "PUERTO_MATE");
    
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    mateStruct->pid = (uint32_t)time.tv_nsec;

    bool isDebug = config_get_int_value(mateStruct->mateConfig, "DEBUG");
    int loggerLevel;
    if(isDebug) loggerLevel = LOG_LEVEL_DEBUG;
    else loggerLevel = LOG_LEVEL_INFO;
    char* loggerFileName = string_from_format("mate_%u.log", mateStruct->pid);
    char* mateName = string_from_format("Mate %u", mateStruct->pid);
    mateStruct->logger = log_create(loggerFileName, mateName, isDebug, loggerLevel);
    free(loggerFileName);
    free(mateName);

    mateStruct->mateSocket = connectToServer(mateStruct->mateIP, mateStruct->matePort);
    mateStruct->isMemory = (socket_getHeader(mateStruct->mateSocket) == ID_MEMORIA);
    t_packet* ID = createPacket(CAPI_ID, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(ID->payload, mateStruct->pid);
    socket_sendPacket(mateStruct->mateSocket, ID);
    destroyPacket(ID);

    if(mateStruct->isMemory)
        log_debug(mateStruct->logger, "Mate creado y conectado directamente a memoria");
    else log_debug(mateStruct->logger, "Mate creado y conectado al kernel");

    return 0;
}

int mate_close(mate_instance *lib_ref){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    
    t_packet* packet = createPacket(CAPI_TERM, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(packet->payload, mateStruct->pid);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    log_debug(mateStruct->logger, "Avisando terminacion");

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);

    packet = createPacket(DISCONNECTED, 0);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    log_debug(mateStruct->logger, "Avisando desconexion");

    packet = socket_getPacket(mateStruct->mateSocket);
    rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);    

    log_debug(mateStruct->logger, "Mate cerrado");
    
    close(mateStruct->mateSocket);
    config_destroy(mateStruct->mateConfig);
    log_destroy(mateStruct->logger);
    free(mateStruct);

    return rc;
}

//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(SEM_INIT, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    streamAdd_UINT32(packet->payload, (uint32_t)value);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);

    if(rc) log_debug(mateStruct->logger, "Semaforo %s no pudo ser creado", sem);
    else log_debug(mateStruct->logger, "Semaforo %s creado", sem);

    return rc;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    log_debug(mateStruct->logger, "Por esperar al Semaforo %s", sem);

    t_packet* packet = createPacket(SEM_WAIT, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    
    if(rc) {
        log_debug(mateStruct->logger, "Error de deadlock al esperar al semaforo, el mate se ha cerrado %s", sem);
        close(mateStruct->mateSocket);
        config_destroy(mateStruct->mateConfig);
        log_destroy(mateStruct->logger);
        free(mateStruct);
        lib_ref->group_info = NULL;
    }
    else log_debug(mateStruct->logger, "Termino la espera en semaforo %s", sem);

    return rc;
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(SEM_POST, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);

    if(rc) log_debug(mateStruct->logger, "Semaforo %s no pudo ser posteado", sem);
    else log_debug(mateStruct->logger, "Semaforo %s posteado", sem);
    
    return rc;
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(SEM_DESTROY, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);

    if(rc) log_debug(mateStruct->logger, "Semaforo %s no pudo ser destruido", sem);
    else log_debug(mateStruct->logger, "Semaforo %s destruido", sem);

    return rc;
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    log_debug(mateStruct->logger, "Por usar al dispositivo IO %s", io);

    t_packet* packet = createPacket(CALL_IO, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, io);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);

    if(rc) log_debug(mateStruct->logger, "Error al querer usar el dispositivo IO %s", io);
    else log_debug(mateStruct->logger, "Termino el uso del dispositivo IO %s", io);

    return rc;
}

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size){
    if(lib_ref->group_info == NULL) return 0;
    mate_inner_structure* mateStruct = (mate_inner_structure*) (lib_ref->group_info);

    log_debug(mateStruct->logger, "Por pedir %i bytes de memoria.", size);

    t_packet* packet = createPacket(MEMALLOC, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(packet->payload, mateStruct->pid);
    streamAdd_UINT32(packet->payload, (uint32_t)size);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    log_debug(mateStruct->logger, "Se pidieron los %i bytes.", size);

    packet = socket_getPacket(mateStruct->mateSocket);
    mate_pointer result = streamTake_INT32(packet->payload);
    destroyPacket(packet);
    log_debug(mateStruct->logger, "Resultado: %u.", result);


    if(result == -1) log_debug(mateStruct->logger, "Fallo al pedir memoria");
    else log_debug(mateStruct->logger, "Memoria asignada en direccion logica %i", result);

    return result;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;

    log_debug(mateStruct->logger, "Por pedir hacer free de la direccion %i", addr);

    t_packet* packet = createPacket(MEMFREE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(packet->payload, mateStruct->pid);
    streamAdd_INT32(packet->payload, addr);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : MATE_FREE_FAULT;
    destroyPacket(packet);

    if(rc) log_debug(mateStruct->logger, "Fallo al hacer free");
    else log_debug(mateStruct->logger, "Memoria liberada");

    return rc;
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;

    log_debug(mateStruct->logger, "Por leer %i bytes de la direccion logica %i", size, origin);

    t_packet* packet = createPacket(MEMREAD, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(packet->payload, mateStruct->pid);
    streamAdd_INT32(packet->payload, origin);
    streamAdd_INT32(packet->payload, (int32_t)size);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);


    packet = socket_getPacket(mateStruct->mateSocket);
    if(packet->header == ERROR){
        log_debug(mateStruct->logger, "Fallo al leer");
        destroyPacket(packet);
        return MATE_READ_FAULT;
    }

    void *recvd = NULL;
    int32_t recvdSize = streamTake_INT32(packet->payload);
    printf("Rcvd size: %i\n", recvdSize);
    streamTake(packet->payload, &recvd, (size_t)recvdSize);
    memcpy(dest, recvd, recvdSize);
    destroyPacket(packet);

    log_debug(mateStruct->logger, "Memoria leida");

    return 0;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size){
    if(lib_ref->group_info == NULL) return -1;
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;

    log_debug(mateStruct->logger, "Por escribir %i bytes en la direccion logica %i", size, dest);

    t_packet* packet = createPacket(MEMWRITE, INITIAL_STREAM_SIZE);
    streamAdd_UINT32(packet->payload, mateStruct->pid);
    streamAdd_INT32(packet->payload, dest);
    streamAdd_INT32(packet->payload, (int32_t)size);
    streamAdd(packet->payload, origin, (size_t)size);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    log_debug(mateStruct->logger, "Enviada peticion para escrbir %i bytes en la direccion logica %i", size, dest);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : MATE_WRITE_FAULT;
    destroyPacket(packet);

    log_debug(mateStruct->logger, "Recibida respuesta de escritura");

    if(rc) log_debug(mateStruct->logger, "Fallo al escribir");
    else log_debug(mateStruct->logger, "Memoria escrita");

    return rc;
}