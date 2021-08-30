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
    socket_send(socket, (void*)&header, sizeof(uint8_t));
    socket_send(socket, (void*)&packet->data->offset, sizeof(uint32_t));
    socket_send(socket, (void*)packet->data->stream, packet->data->offset);
}

void socket_get(int socket, void* dest, size_t size){
    guard_syscall(recv(socket, dest, size, 0));
}

msgHeader socket_getHeader(int socket){
    uint8_t header;
    socket_get(socket, &header, sizeof(uint8_t));
    return (msgHeader)header;
}

t_packet* socket_getPacket(int socket){
    msgHeader header = socket_getHeader(socket);
    uint32_t streamSize;
    socket_get(socket, &streamSize, sizeof(uint32_t));
    t_packet* packet = createPacket(streamSize);
    socket_get(socket, packet->data->stream, streamSize);
    packet->header = header;
    return packet;
}

int connectToServer(char* serverIp, char* serverPort){
	int clientSocket = 0;
    struct addrinfo hints;
	struct addrinfo *serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	guard_syscall(getaddrinfo(serverIp, serverPort, &hints, &serverInfo));
	guard_syscall(clientSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol));
	guard_syscall(connect(clientSocket, serverInfo->ai_addr, serverInfo->ai_addrlen));
	freeaddrinfo(serverInfo);
	return clientSocket;
}

int createListenServer(char* serverIP, char* serverPort){
    int serverSocket = 0;
    struct addrinfo hints;
    struct addrinfo *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(serverIP, serverPort, &hints, &serverInfo);
	guard_syscall(serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol));
	guard_syscall(bind(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen));
	guard_syscall(listen(serverSocket, MAX_CLIENTS));
    freeaddrinfo(serverInfo);
    return serverSocket;
}

int* getNewClient(int serverSocket){
    int* newClientSocket = malloc(sizeof(int));
    struct sockaddr_in clientAddr;
	socklen_t addrSize = sizeof(struct sockaddr_in);
    guard_syscall(*newClientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize));
    return newClientSocket;
}

//TODO deberia tomar un cliente recien agregado, crear un thread y entregarselo para que lo atienda
void clientDispatcher(int* clientSocket){
    
}