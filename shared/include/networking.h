#ifndef NETWORKING_H_
#define NETWORKING_H_

#include "serialize.h"
#include "guards.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_CLIENTS 100

/**
 * @DESC: Los posibles headers para comunicarse por socket
 */
typedef enum msgHeader { PING, ACK, DISCONNECTED } msgHeader;

/**
 * @DESC: Contiene:
 *          - el header del mensaje
 *          - un streamBuffer con el mensaje
 */
typedef struct packet {
    msgHeader header;
    t_streamBuffer* data;
} t_packet;

/**
 * @DESC: Crea un objeto Packet en memoria
 * @param size: tamanio alojado al stream que contiene
 * @return t_packet*: puntero al packet creado
 */
t_packet* createPacket(size_t size);

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
 * @DESC: Envia un packet al socket
 * 
 * formato: [ header | tamanio del stream | stream ]
 * 
 * @param socket: socket a enviar el packet
 * @param packet: puntero al packet
 */
void socket_sendPacket(int socket, t_packet* packet);

/**
 * @DESC: (wrapper) recv sin flags con guarda
 * @param socket: socket del cual recibir los datos
 * @param dest: puntero al cual se alojaran los datos
 * @param size: tamanio de los datos (el puntero debe tener alojado ese tamanio minimo)
 */
void socket_get(int socket, void* dest, size_t size);

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
int* getNewClient(int serverSocket);

#endif // !NETWORKING_H_
