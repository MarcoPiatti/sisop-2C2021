#ifndef NETWORKING_H_
#define NETWORKING_H_

#include "serialize.h"
#include <sys/socket.h>

typedef enum msgHeader { HANDSHAKE, PING, ACK, DISCONNECTED } msgHeader;

void socket_send(int socket, void* source, size_t size);

void socket_sendPacket(int socket, msgHeader header, streamBuffer* stream);

void socket_read(int socket, void* dest, size_t size);

#endif // !NETWORKING_H_
