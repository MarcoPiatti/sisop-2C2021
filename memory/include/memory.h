#ifndef MEMORY_H_
#define MEMORY_H_

#include "networking.h"
#include "swapInterface.h"
#include "memoryConfig.h"
#include "ram.h"
#include "tlb.h"
#include "pageTable.h"
#include "logs.h"
#include <string.h>
#include <unistd.h>

typedef struct HeapMetadata {
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isFree;
} t_HeapMetadata;

t_memoryConfig* memoryConfig;

t_swapInterface* swapInterface;

t_TLB* tlb;

t_ram* ram;

t_pageTable* pageTable;

//TODO: Implementar algun tipo de sincronizacion entre las TADs de ram, pageTable y TLB o por afuera.
//Por ahora lo dejo modo croto con un mutex gigante que acapara toda la operacion de un saque.
pthread_mutex_t mutexMemoria;

extern bool (*petitionHandlers[MAX_PETITIONS])(int clientSocket, t_packet* petition);

/**
 * @DESC: Funcion para ubicar en MP una pagina previamente existente
 */
int32_t getFrame(uint32_t pid, int32_t page);

/**
 * @DESC: Lee "size" bytes del heap de un programa. Abstrayendo de la idea de paginas.
 * @param pid: PID del programa
 * @param logicAddress: direccion logica de donde empezar a leer
 * @param size: cantidad de bytes a leer del heap
 * @return void*: puntero malloc'd (con size bytes) con los datos solicitados
 */
void* heap_read(uint32_t pid, int32_t logicAddress, int size);

/**
 * @DESC: Idem heap_read, pero para escribir
 * @param pid: PID del programa
 * @param logicAddress: direccion logica a escribir
 * @param size: cantidad de bytes
 * @param data: puntero con los datosbool createPage(uint32_t pid, void* data)
 */
void heap_write(uint32_t pid, int32_t logicAddress, int size, void* data);

/**
 * @DESC: Crea una pagina nueva para un proceso y la aloja en MP o swap, dependiendo de los cupos libres
 * @param pid: PID del proceso que necesita una pagina nueva
 * @param data: puntero a los datos con los que se creara la nueva pagina, normalmente vacia, la primera lleva un alloc inicial
 * @return true: retorna true si se pudo crear la pagina
 * @return false: retorna false si no se pudo crear la pagina
 */
bool createPage(uint32_t pid, void* data);

//Funciones handler para seniales
void handlerUSR1(int num);
void handlerUSR2(int num);
void handlerINT(int num);

//TODO: Revisar que si en un memalloc se deben crear muchas paginas, y solo la ultima fallo
// Se destruyan las anteriores que si fueron creadas, ya que no hacen nada.
// O que se queden ahi como un recurso gastado y listo

#endif // !MEMORY_H_