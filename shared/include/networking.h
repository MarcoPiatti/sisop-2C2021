#ifndef NETWORKING_H_
#define NETWORKING_H_

#include "serialize.h"
#include "guards.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_BACKLOG 100
//TODO: Implementar mensaje donde un carpincho presenta su ID
//TODO: Implementar mensaje donde un carpincho avisa que se quiere dar de baja en memoria
/**
 * @DESC: Los posibles headers para comunicarse por socket
 */
typedef enum msgHeader {
    /* Peticiones de los carpinchos, respetar orden *//* Formato de los mensajes serializados */
    SEM_INIT,           // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING | SEM_VALUE = UINT32 |
    SEM_WAIT,           // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING |
    SEM_POST,           // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING |
    SEM_DESTROY,        // | HEADER | PAYLOAD_SIZE | SEM_NAME = STRING |
    CALL_IO,            // | HEADER | PAYLOAD_SIZE | IO_NAME = STRING  |
    MEMALLOC,           // | HEADER | PAYLOAD_SIZE | SIZE = INT32 |
    MEMFREE,            // | HEADER | PAYLOAD_SIZE | PTR = INT32 |
    MEMREAD,            // | HEADER | PAYLOAD_SIZE | PTR = INT32 | SIZE = INT32 |
    MEMWRITE,           // | HEADER | PAYLOAD_SIZE | PTR = INT32 | DATASIZE = INT32 | DATA = STREAM |
    DISCONNECTED,       // | HEADER | PAYLOAD_SIZE = 0 |
    MAX_PETITIONS,
    /* Respuestas a carpinchos*/ 
    ID_KERNEL,          // | HEADER | 
    ID_MEMORIA,         // | HEADER |
    OK,                 // | HEADER | PAYLOAD_SIZE = 0 |
    ERROR,              // | HEADER | PAYLOAD_SIZE = 0 |
    POINTER,            // | HEADER | PAYLOAD_SIZE | POINTER = INT32 |
    MEM_CHUNK           // | HEADER | PAYLOAD_SIZE | DATA = STREAM |
} msgHeader;

/**
 * @DESC: Contiene:
 *          - el header del mensaje
 *          - un streamBuffer con el mensaje
 */
typedef struct packet {
    uint8_t header;
    t_streamBuffer* payload;
} t_packet;

/**
 * @DESC: Crea un objeto Packet en memoria, a partir de un header y tamanio de payload inicial
 * @param header: header del mensaje
 * @param size: tamanio alojado al stream que contiene
 * @return t_packet*: puntero al packet creado
 */
t_packet* createPacket(uint8_t header, size_t size);

/**
 * @DESC: Destruye un packet de memoria
 * @param packet: puntero al packet a destruir
 */
void destroyPacket(t_packet* packet);

/**
 * @DESC: (wrapper) send sin flags con guarda
 * @param socket: socket a enviar datos
 * @param source: puntero a los datos enviados
 * @param size: tamanio de los datos enviados
 */
void socket_send(int socket, void* source, size_t size);

/**
 * @DESC: Envia unicamente un header al socket
 * @param socket: socket para enviar
 * @param header: header enviado
 */
void socket_sendHeader(int socket, uint8_t header);

/**
 * @DESC: Envia un packet al socket
 * 
 * formato: [ header | tamanio del stream | stream ]
 * 
 * @param socket: socket a enviar el packet
 * @param packet: puntero al packet
 */
void socket_sendPacket(int socket, t_packet* packet);

/**
 * @DESC: Reenvia un packet previamente recibido a un socket
 * @param socket: socket destinatario
 * @param packet: packet previamente recibido
 */
void socket_relayPacket(int socket, t_packet* packet);

/**
 * @DESC: (wrapper) recv sin flags con guarda
 * @param socket: socket del cual recibir los datos
 * @param dest: puntero al cual se alojaran los datos
 * @param size: tamanio de los datos (el puntero debe tener alojado ese tamanio minimo)
 */
void socket_get(int socket, void* dest, size_t size);

/**
 * @DESC: Obtiene unicamente un header del socket
 * @param socket: socket del cual se obtiene el header
 * @return msgHeader: header obtenido
 */
uint8_t socket_getHeader(int socket);

/**
 * @DESC: Obtiene un packet del socket
 * @param socket: socket del cual se obtiene el packet
 * @return t_packet*: puntero al packet obtenido
 */
t_packet* socket_getPacket(int socket);

/**
 * @DESC: Se conecta a un server y crea un socket
 * @param ip: ip del server
 * @param port: puerto del server
 * @return int: socket del cliente conectado al server
 */
int connectToServer(char* serverIp, char* serverPort);

/**
 * @DESC: Crea un servidor para escuchar conexiones
 * @param port: puerto del servidor
 * @return int: socket del servidor 
 */
int createListenServer(char* serverIP, char* serverPort);

/**
 * @DESC: queda a la espera de que se conecte un nuevo cliente
 * @param socket: retorna un puntero al nuevo socket del cliente conectado
 */
int getNewClient(int serverSocket);

/**
 * @DESC: Eternamente recibe clientes y los delega a un thread que los atiende
 * @param serverSocket: socket retornado en createListenServer()
 * @param clientHandler: funcion para atender clientes que los threads va a usar
 */
void runListenServer(int serverSocket, void*(*clientHandler)(void*));

#endif // !NETWORKING_H_
