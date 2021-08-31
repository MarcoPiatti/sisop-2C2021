#include "server.h"

#include "networking.h"
#include "serialize.h"
#include <readline/readline.h>
#include <commons/log.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_IP "192.168.0.3" 
#define SERVER_PORT "4444"

pthread_mutex_t mutexLog = PTHREAD_MUTEX_INITIALIZER, mutexSocket = PTHREAD_MUTEX_INITIALIZER;

void* leerSocket(void* args){
    int clientSocket = *(int*)args;
    char* msjRecibido;
    pthread_mutex_unlock(&mutexSocket);
    t_packet* paqueteRecibido = socket_getPacket(clientSocket);
    pthread_mutex_lock(&mutexSocket);
    while(paqueteRecibido->header != DISCONNECTED){
        switch (paqueteRecibido -> header){
        case STRING:
            msjRecibido = streamTake_STRING(paqueteRecibido->data);
            pthread_mutex_unlock(&mutexLog);
            log_info(logger, "Marco: %s", msjRecibido);
            pthread_mutex_unlock(&mutexLog);
            free(msjRecibido);
            break;
        default:
            break;
        }
        destroyPacket(paqueteRecibido);
        pthread_mutex_unlock(&mutexSocket);
        paqueteRecibido = socket_getPacket(clientSocket);
        pthread_mutex_lock(&mutexSocket);
    }
    pthread_exit((void*)0);
}


int main(){

    logger = log_create("./cfg/logsito.log", "SERVER", 1, LOG_LEVEL_INFO);

    int serverSocket = createListenServer(SERVER_IP, SERVER_PORT);
    int* clientSocket = getNewClient(serverSocket);
        

    pthread_t lector = 0; 
    pthread_create(&lector, NULL, leerSocket, (void*)clientSocket);

    char* leido = readline(" > ");

    while(strcmp(leido, "") != 0){
        t_packet* paqueton = createPacket(INITIAL_STREAM_SIZE);
        paqueton->header = STRING;
        streamAdd_STRING(paqueton->data, leido);
        pthread_mutex_unlock(&mutexSocket);
        socket_sendPacket(*clientSocket, paqueton);
        pthread_mutex_lock(&mutexSocket);

        pthread_mutex_unlock(&mutexLog);
        log_info(logger, "Ulises: %s", leido);
        pthread_mutex_unlock(&mutexLog);

        destroyPacket(paqueton);
        free(leido);

        leido = readline(" > ");
    }

    free(leido);

    t_packet* paquetonto = createPacket(0);
    paquetonto->header = DISCONNECTED;

    pthread_mutex_unlock(&mutexSocket);
    socket_sendPacket(*clientSocket, paquetonto);
    pthread_mutex_lock(&mutexSocket);

    destroyPacket(paquetonto);

    int retorno = 0;
    pthread_join(lector, (void**)&retorno);

    close(*clientSocket);
    log_destroy(logger);


    return 0;
}