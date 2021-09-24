#ifndef SWAPINTERFACE_H_
#define SWAPINTERFACE_H_

#include "networking.h"
#include "headers.h"
#include <pthread.h>

typedef struct swapInterface {
    int pageSize;
    int socket;
    pthread_mutex_t mutex;
} t_swapInterface;

/**
 * @DESC: Crea una interfaz hacia un swap
 * @param swapIp: Ip del swap
 * @param swapPort: Puerto del swap
 * @param pageSize: Tamanio de las paginas a enviar al swap
 * @return t_swapInterface*: La nueva interfaz creada
 */
t_swapInterface* swapInterface_create(char* swapIp, char* swapPort, int pageSize, swapHeader algorithm);

/**
 * @DESC: Envia una pagina de un proceso al swap para guardarla
 * @param self: Interfaz usada
 * @param pid: ID del proceso duenio de la pagina
 * @param pageNumber: Nro de pagina en el contexto del proceso
 * @param pageContent: Puntero a los datos de la pagina
 * @return true: Retorna true si se pudo guardar la pagina :)
 * @return false: Retorna false si NO se pudo guardar la pagina :(
 */
bool swapInterface_savePage(t_swapInterface* self, uint32_t pid, int32_t pageNumber, void* pageContent);

/**
 * @DESC: Pide al swap los datos de una pagina, a partir de su identificacion
 * @param self: Interfaz usada
 * @param pid: ID del proceso duenio de la pagina
 * @param pageNumber: Nro. de la pagina en el contexto del proceso
 * @return void*: Puntero a los datos de la pagina, NULL si no se pudo conseguir
 */
void* swapInterface_loadPage(t_swapInterface* self, uint32_t pid, int32_t pageNumber);

/**
 * @DESC: Le pide al swap que borre todos los datos de un proceso dado
 * @param self: Interfaz usada
 * @param pid: ID del proceso a eliminar de swap
 * @return true: Retorna true si el proceso se borro sin problemas :)
 * @return false: Retorna false si hubo algun problema (no deberia)
 */
bool swapInterface_eraseProcess(t_swapInterface* self, uint32_t pid);

/**
 * @DESC: Destruye una interfaz a un swap
 */
void swapInterface_destroy(t_swapInterface* self);

#endif // !SWAPINTERFACE_H_
