#ifndef IODEVICE_H_
#define IODEVICE_H_

#include <pthread.h>
#include "pQueue.h"

/**
 * @DESC: TAD de dispositivo I/O. tiene un nombre y duracion de consulta.
 *        Tiene una cola de espera para los dispositivos que lo deban usar-
 *        Y un thread que procesara los dispositivos de la cola de espera.
 */
typedef struct IODevice {
    char* nombre;
    int duracion;
    t_pQueue* waitingProcesses;
    pthread_t thread_IODevice;
} t_IODevice;

/**
 * @DESC: Crea un Dispositivo I/O
 * @param nombre: nombre del dispositivo 
 * @param duracion: duracion de ejecucion del dispositivo
 * @param IOfunc: funcion que ejecutara el thread del dispositivo
 * @return t_IODevice*: puntero al struct del dispoitivo
 */
t_IODevice* createIODevice(char* nombre, int duracion, void* (* IOfunc)(void*));

/**
 * @DESC: Destruye un Dispositivo I/O, su thread y su cola de espera
 * @param IODevice: dispositivo a destruir
 */
void destroyIODevice(t_IODevice* IODevice);

#endif // !IODEVICE_H_