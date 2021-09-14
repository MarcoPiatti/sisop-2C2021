#include "matelib.h"

#include "networking.h"
#include <commons/config.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct mate_inner_structure{ //TODO preguntar para que se necesita un identificador (UUID o PID, etc)
    t_config* mateConfig;
    char* mateIP;
    char* matePort;
    int mateSocket;
    bool isMemory;
} mate_inner_structure;

//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config){
    lib_ref->group_info = malloc(sizeof(mate_inner_structure));
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    mateStruct->mateConfig = config_create(config);
    mateStruct->mateIP = config_get_string_value(mateStruct->mateConfig, "IP_MATE");
    mateStruct->matePort = config_get_string_value(mateStruct->mateConfig, "PUERTO_MATE");
    mateStruct->mateSocket = connectToServer(mateStruct->mateIP, mateStruct->matePort);
    mateStruct->isMemory = (socket_getHeader(mateStruct->mateSocket) == ID_MEMORIA);
    return 0;
}

int mate_close(mate_instance *lib_ref){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    t_packet* packet = createPacket(DISCONNECTED, 0);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;

    close(mateStruct->mateSocket);
    config_destroy(mateStruct->mateConfig);
    return rc;
}

//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value){
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
    return rc;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(SEM_WAIT, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    return rc;
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(SEM_POST, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    return rc;
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(SEM_DESTROY, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, sem);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    return rc;
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    if (mateStruct->isMemory) return 1;

    t_packet* packet = createPacket(CALL_IO, INITIAL_STREAM_SIZE);
    streamAdd_STRING(packet->payload, io);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    return rc;
}

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;
    
    t_packet* packet = createPacket(MEMALLOC, INITIAL_STREAM_SIZE);
    streamAdd_INT32(packet->payload, (int32_t)size);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    mate_pointer result = streamTake_INT32(packet->payload);
    destroyPacket(packet);
    return result;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;

    t_packet* packet = createPacket(MEMFREE, INITIAL_STREAM_SIZE);
    streamAdd_INT32(packet->payload, addr);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    return rc;
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;

    t_packet* packet = createPacket(MEMREAD, INITIAL_STREAM_SIZE);
    streamAdd_INT32(packet->payload, origin);
    streamAdd_INT32(packet->payload, (int32_t)size);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    if(packet->header == ERROR){
        destroyPacket(packet);
        return -1;
    }
    streamTake(packet->payload, &dest, (size_t)size);
    destroyPacket(packet);
    return 0;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size){
    mate_inner_structure* mateStruct = (mate_inner_structure*)lib_ref->group_info;

    t_packet* packet = createPacket(MEMWRITE, INITIAL_STREAM_SIZE);
    streamAdd_INT32(packet->payload, dest);
    streamAdd_INT32(packet->payload, (int32_t)size);
    streamAdd(packet->payload, &origin, (size_t)size);
    socket_sendPacket(mateStruct->mateSocket, packet);
    destroyPacket(packet);

    packet = socket_getPacket(mateStruct->mateSocket);
    int rc = (packet->header == OK) ? 0 : -1;
    destroyPacket(packet);
    return rc;
}