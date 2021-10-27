#include <kernel.h>

processState _capi_id(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _sem_init(t_packet *received, int clientSocket){
    char* semName = streamTake_STRING(received->payload);
    uint32_t semCount = streamTake_UINT32(received->payload);
    //t_mateSem* newSem = mateSem_create(semName, semCount, /**/ );

    return CONTINUE;
}

processState _sem_wait(t_packet *received, int clientSocket){
    
    return BLOCK;    //todo: retornar solo si es menor a 0
}

processState _sem_post(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _sem_destroy(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _call_io(t_packet *received, int clientSocket){
    return BLOCK;    //todo: ver
}

processState _memalloc(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _memfree(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _memread(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _memwrite(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _capi_term(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState _disconnected(t_packet *received, int clientSocket){
    return CONTINUE;
}

processState (*petitionProcessHandler[MAX_PETITIONS])(t_packet *received, int clientSocket) = {
    _capi_id,
    _sem_init,
    _sem_wait,
    _sem_post,
    _sem_destroy,
    _call_io,
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
