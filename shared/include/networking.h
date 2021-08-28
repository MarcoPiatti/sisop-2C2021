#ifndef NETWORKING_H_
#define NETWORKING_H_

#include "serialize.h"
#include "guards.h"
#include <sys/socket.h>

typedef enum msgHeader { HANDSHAKE, PING, ACK, DISCONNECTED } msgHeader;

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
int connectToServer(char* serverIp, char* serverPort); //TODO Desarrollar

/**
 * @DESC: Crea un servidor para escuchar conexiones
 * @param port: puerto del servidor
 * @return int: socket del servidor 
 */
int createListenServer(char* serverPort); //TODO Desarrollar

/**
 * @DESC: queda a la escucha de nuevas conexiones
 * @param socket: retorna un socket al primer cliente en conectarse
 */
void listen(int clientSocket); //TODO Desarrollar

#endif // !NETWORKING_H_
