#include "networking.h"

t_packet* createPacket(uint8_t header, size_t size){
    t_packet* tmp = malloc(sizeof(t_packet));
    tmp->header = header;
    tmp->payload = createStream(size);
    return tmp;
}

void destroyPacket(t_packet* packet){
    destroyStream(packet->payload);
    free(packet);
}

void socket_send(int socket, void* source, size_t size){
    guard_syscall(send(socket, source, size, 0));
}

void socket_sendHeader(int socket, uint8_t header){
    uint8_t tmpHeader = header;
    socket_send(socket, (void*)&tmpHeader, sizeof(uint8_t));
}

void socket_sendPacket(int socket, t_packet* packet){
    socket_sendHeader(socket, packet->header);
    socket_send(socket, (void*)&packet->payload->offset, sizeof(uint32_t));
    socket_send(socket, (void*)packet->payload->stream, packet->payload->offset);
}

void socket_relayPacket(int socket, t_packet* packet){
    packet->payload->offset = (uint32_t)packet->payload->mallocSize;
    socket_sendPacket(socket, packet);
}

void socket_get(int socket, void* dest, size_t size){
    if(size != 0) guard_syscall(recv(socket, dest, size, 0));
}

uint8_t socket_getHeader(int socket){
    uint8_t header;
    socket_get(socket, &header, sizeof(uint8_t));
    return header;
}

t_packet* socket_getPacket(int socket){
    uint8_t header = socket_getHeader(socket);
    uint32_t streamSize;
    socket_get(socket, &streamSize, sizeof(uint32_t));
    t_packet* packet = createPacket(header, streamSize);
    socket_get(socket, packet->payload->stream, streamSize);
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
    guard_syscall(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)));
	guard_syscall(bind(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen));
	guard_syscall(listen(serverSocket, MAX_BACKLOG));
    freeaddrinfo(serverInfo);
    return serverSocket;
}

int getNewClient(int serverSocket){
    int newClientSocket = 0;
    struct sockaddr_in clientAddr;
	socklen_t addrSize = sizeof(struct sockaddr_in);
    guard_syscall(newClientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize));
    return newClientSocket;
}

void runListenServer(int serverSocket, void*(*clientHandler)(void*)){
    int newClient = getNewClient(serverSocket);
    pthread_t clientHandlerThread = 0;
    guard_syscall(pthread_create(&clientHandlerThread, NULL, clientHandler, (void*)newClient));
    pthread_detach(clientHandlerThread);
}