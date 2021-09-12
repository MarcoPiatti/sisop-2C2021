#include "kernel.h"

void* thread_longTermFunc(void* args){
    t_process *process;
    while(1){
        sem_wait(&sem_multiprogram);
        if(pQueue_isEmpty(suspendedReadyQueue))
            process = pQueue_take(newQueue);
        else process = pQueue_take(suspendedReadyQueue);
    }
}

void* thread_mediumTermFunc(void* args){
    //TODO: Desarrollar funcion medium term
}

void* thread_CPUFunc(void* args){
    //TODO: Desarrollar funcion CPU
}

void* thread_IODeviceFunc(void* args){
    //TODO: Desarrollar funcion IODevices
}

void* thread_semFunc(void* args){
    //TODO: Desarrollar funcion semaforos
}

int main(){
    kernelConfig = getKernelConfig("./cfg/kernel.config");

    /* Inicializar estructuras de estado */
    newQueue = pQueue_create();
    readyQueue = pQueue_create();
    blockedQueue = pQueue_create();
    suspendedReadyQueue = pQueue_create();

    /* Inicializar semaforo de multiprocesamiento */
    sem_init(&sem_multiprogram, 0, kernelConfig->multiprogram);

    /* Inicializar CPUs */
    thread_CPUs = malloc(sizeof(pthread_t) * kernelConfig->multiprocess);
    for(int i = 0; i < kernelConfig->multiprocess; i++){
        pthread_create(thread_CPUs + i, 0, thread_CPUFunc, NULL);
        pthread_detach(thread_CPUs[i]);
    }

    /* Inicializar Dispositivos IO */
    IO_dict = dictionary_create();
    for(int i = 0; kernelConfig->IODeviceNames[i]; i++){
        dictionary_put( IO_dict,
                        kernelConfig->IODeviceNames[i],
                        createIODevice( kernelConfig->IODeviceNames[i],
                                        kernelConfig->IODeviceDelays[i],
                                        thread_IODeviceFunc));
    }

    /* Crear estructura para agregar semaforos */
    sem_dict = dictionary_create();

    /* Inicializar Planificador de largo plazo */
    pthread_create(&thread_longTerm, 0, thread_longTermFunc, NULL);
    pthread_detach(thread_longTerm);
    
    /* Inicializar Planificador de mediano plazo */
    pthread_create(&thread_mediumTerm, 0, thread_mediumTermFunc, NULL);
    pthread_detach(thread_mediumTerm);

    /* Inicializar servidor */
    int serverSocket = createListenServer(kernelConfig->kernelIP, kernelConfig->kernelPort);
    
    while(1){
        int newProcessSocket = getNewClient(serverSocket);
        socket_sendHeader(newProcessSocket, ID_KERNEL);
        pQueue_put(newQueue, createProcess(newProcessSocket, newProcessSocket));
    }

    return 0;
}