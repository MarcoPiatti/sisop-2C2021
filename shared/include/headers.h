#ifndef MSGHEADERS_H_
#define MSGHEADERS_H_

/**
 * @DESC: Los posibles headers para comunicarse por socket
 */

typedef enum capiHeader {
    /* Peticiones de los carpinchos, respetar orden *//* Formato de los mensajes serializados */
    ID_CAPI,            // | HEADER | PAYLOAD_SIZE | PID = UINT32 |
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
    CAPI_SUSP,
    MAX_PETITIONS,
    /* Respuestas a carpinchos*/ 
    ID_KERNEL,          // | HEADER | 
    ID_MEMORIA,         // | HEADER |
    OK,                 // | HEADER | PAYLOAD_SIZE = 0 |
    ERROR,              // | HEADER | PAYLOAD_SIZE = 0 |
    POINTER,            // | HEADER | PAYLOAD_SIZE | POINTER = INT32 |
    MEM_CHUNK           // | HEADER | PAYLOAD_SIZE | DATASIZE = INT32 | DATA = STREAM |
} capiHeader;

typedef enum swapHeader {
    SAVE_PAGE,          // | HEADER | PAYLOAD_SIZE | PID = UINT32 | PAGE_N = INT32 | PAGE_DATA = PAGE_SIZE |
    LOAD_PAGE,          // | HEADER | PAYLOAD_SIZE | PID = UINT32 | PAGE_N = INT32 |
    CAPI_ERASE,         // | HEADER | PAYLOAD_SIZE | PID = UINT32 |
    MEM_DISCONNECT,       // | HEADER | PAYLOAD_SIZE = 0 |
    MAX_MEM_MSGS,
    ASIGN_FIJO,
    ASIGN_GLOBAL,
    OK_MEM,                 // | HEADER | PAYLOAD_SIZE = 0 |
    ERROR_MEM,              // | HEADER | PAYLOAD_SIZE = 0 |
    PAGE_DATA           // | HEADER | PAYLOAD_SIZE | PAGE_DATA = PAGE_SIZE |
} swapHeader;

#endif // !MSGHEADERS_H_