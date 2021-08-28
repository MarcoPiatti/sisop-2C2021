#include "networking.h"

t_packet* createPacket(size_t size){
    t_packet* tmp = malloc(sizeof(t_packet));
    tmp->data = createStream(size);
    return tmp;
}

void destroyPacket(t_packet* packet){
    destroyStream(packet->data);
    free(packet);
}

void socket_send(int socket, void* source, size_t size){
    guard_syscall(send(socket, source, size, 0));
}

void socket_sendPacket(int socket, t_packet* packet){
    uint8_t header = packet->header;
    socket_send(socket, packet->header, sizeof(uint8_t));
    socket_send(socket, packet->data->offset, sizeof(uint32_t));
    socket_send(socket, packet->data->stream, packet->data->offset);
}

void socket_get(int socket, void* dest, size_t size){
    guard_syscall(recv(socket, dest, size, 0));
}

t_packet* socket_getPacket(int socket){
    uint8_t header;
    uint32_t streamSize;
    socket_get(socket, &header, sizeof(uint8_t));
    socket_get(socket, &streamSize, sizeof(uint32_t));
    t_packet* packet = createPacket(streamSize);
    socket_get(socket, packet->data->stream, streamSize);
    return packet;
}