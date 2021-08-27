#include "networking.h"

void socket_send(int socket, void* source, size_t size){
    send(socket, source, size, 0);
}

void socket_pack(int socket, msgHeader header, streamBuffer* stream){
    streamBuffer* packet = createStream_S();
    streamAdd_UINT8(packet, (void*)&header);
    streamAdd_STREAM(packet, stream);

}

void socket_read(int socket, void* dest, size_t size);

void socket_unpack(int socket, msgHeader* header, streamBuffer* stream);