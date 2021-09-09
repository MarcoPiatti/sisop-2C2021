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
    pthread_detach(IODevice->thread_IODevice);
    return IODevice;
}

void destroyIODevice(t_IODevice* IODevice){
    pthread_cancel(IODevice->thread_IODevice);
    pQueue_destroy(IODevice->waitingProcesses, destroyProcess);
    free(IODevice->nombre);
    free(IODevice);
}