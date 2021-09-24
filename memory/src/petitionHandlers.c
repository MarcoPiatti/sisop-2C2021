#include "memory.h"

bool initHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    //TODO: Crear tabla de paginas para el PID solicitado
    return true;
}

bool mallocHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t mallocSize = streamTake_INT32(petition->payload);
    //TODO: Realizar los checkeos correspondientes de si hay espacio
    int32_t matePtr = 0; //Por ahora solo retorna la direccion 0
    t_packet* response = createPacket(POINTER, INITIAL_STREAM_SIZE);
    streamAdd_INT32(response->payload, matePtr);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    return true;
}

bool freeHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    //TODO: Realizar el free correspondiente a partir de matePtr
    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    return true;
}

bool memreadHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    int32_t readSize = streamTake_INT32(petition->payload);

    //TODO: Acceder al puntero y retornar los contenidos en "contents"

    void* contents = "ESTO DEBE SER ALGO EN MEMORIA DE ENSERIO";
    t_packet* response = createPacket(MEM_CHUNK, INITIAL_STREAM_SIZE);
    streamAdd_INT32(response->payload, readSize);
    streamAdd(response->payload, contents, readSize);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    return true;
}

bool memwriteHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    int32_t matePtr = streamTake_INT32(petition->payload);
    int32_t writeSize = streamTake_INT32(petition->payload);
    void* writeData = NULL; 
    streamTake(petition->payload, &writeData, writeSize);

    //TODO: Escribir writeData en la direccion correspondiente

    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    return true;
}

bool terminationHandler(int clientSocket, t_packet* petition){
    uint32_t pid = streamTake_UINT32(petition->payload);
    swapInterface_eraseProcess(swapInterface, pid);

    //TODO: destruir la tabla de paginas del PID

    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    destroyPacket(response);
    return true;
}

bool disconnectionHandler(int clientSocket, t_packet* petition){
    t_packet* response = createPacket(OK, 0);
    socket_sendPacket(clientSocket, response);
    close(clientSocket);
    destroyPacket(response);
    return false;
}

bool (*petitionHandlers[MAX_PETITIONS])(int clientSocket, t_packet* petition) =
{
    initHandler,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mallocHandler,
    freeHandler,
    memreadHandler,
    memwriteHandler,
    terminationHandler,
    disconnectionHandler
};