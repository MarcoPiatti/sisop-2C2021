#include "proceso1.h"

pthread_mutex_t mutexLog = PTHREAD_MUTEX_INITIALIZER, mutexSocket = PTHREAD_MUTEX_INITIALIZER;

void* leerSocket(void* args){
    int clientSocket = *(int*)args;
    char* msjRecibido;
    t_packet* paqueteRecibido = socket_getPacket(clientSocket);
    while(paqueteRecibido->header != DISCONNECTED){
        switch (paqueteRecibido -> header){
        case STRING:
            msjRecibido = streamTake_STRING(paqueteRecibido->payload);
            pthread_mutex_lock(&mutexLog);
            log_info(logger, "Ulises: %s", msjRecibido);
            pthread_mutex_unlock(&mutexLog);
            free(msjRecibido);
            break;
        default:
            break;
        }
        destroyPacket(paqueteRecibido);
        paqueteRecibido = socket_getPacket(clientSocket);
    }
    pthread_exit((void*)0);
}

int main(int argc, char** argv)
{
    if (argc > 1 && strcmp(argv[1], "-test") == 0) {
        run_tests();
        return 0;
    }
    logger = log_create("./cfg/proceso1.log", "PROCESO1", true, LOG_LEVEL_INFO);
    log_info(logger, "Soy el proceso 1!");

    t_config* config = config_create("./cfg/proceso1.config");
    char* serverIP = config_get_string_value(config, "IP");
    char* serverPort = config_get_string_value(config, "PORT");

    int clientSocket = connectToServer(serverIP, serverPort);

    pthread_t lector = 0; 
    guard_syscall(pthread_create(&lector, NULL, leerSocket, (void*)&clientSocket));

    char* leido = readline(">");
    while(strcmp(leido, "") != 0){
        t_packet* paqueton = createPacket_H(STRING);
        paqueton->header = STRING;
        streamAdd_STRING(paqueton->payload, leido);
        socket_sendPacket(clientSocket, paqueton);
        pthread_mutex_lock(&mutexLog);
        log_info(logger, "Marco: %s", leido);
        pthread_mutex_unlock(&mutexLog);
        destroyPacket(paqueton);
        free(leido);
        leido = readline(">");
    }

    free(leido);
    t_packet* paquetonto = createPacket_H(DISCONNECTED);
    socket_sendPacket(clientSocket, paquetonto);
    destroyPacket(paquetonto);
    int retorno = 0;
    pthread_join(lector, (void**)&retorno);
    close(clientSocket);
    log_destroy(logger);
    config_destroy(config);
}