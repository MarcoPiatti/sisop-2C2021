/**
 * @file: IODevice.c
 * @author pepinOS 
 * @DESC: TAD para dispositivos entrada-salida
 * @version 0.1
 * @date: 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "IODevice.h"

#include "process.h"
#include "commons/string.h"
#include <stdlib.h>

t_IODevice* createIODevice(char* nombre, int duracion, void* (* IOfunc)(void*)){
    t_IODevice* IODevice = malloc(sizeof(t_IODevice));
    IODevice->nombre = string_duplicate(nombre);
    IODevice->duracion = duracion;
    IODevice->waitingProcesses = pQueue_create();
    pthread_create(&IODevice->thread_IODevice, 0, IOfunc, (void*)IODevice);
    return IODevice;
}

void destroyIODevice(t_IODevice* IODevice){
    pthread_cancel(IODevice->thread_IODevice);
    pthread_join(IODevice->thread_IODevice, NULL);
    void destroyer(void*elem){
        destroyProcess((t_process*)elem);
    };
    pQueue_destroy(IODevice->waitingProcesses, destroyer);
    free(IODevice->nombre);
    free(IODevice);
}