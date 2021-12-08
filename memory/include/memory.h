#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include "memoryConfig.h"
#include "networking.h"
#include "swapInterface.h"
#include "pageTable.h"
#include <pthread.h>

pthread_mutex_t logMut;
pthread_mutex_t ramMut;
pthread_mutex_t metadataMut;
void initalizeMemMutex();
void destroyMemMutex();

t_swapInterface* swapInterface;
t_memoryConfig *config;
t_log *memLogger;

extern bool (*petitionHandlers[MAX_PETITIONS])(t_packet* petition, int socket);

void *petitionHandler(void *_clientSocket);

// Algoritmo (clock-m o LRU) toma un frame de inicio y un frame final y eligen la victima dentro del rango.
uint32_t (*algoritmo)(int32_t start, int32_t end);

// Asignacion fija o global, devuelven en los parametros un rango de frames entre los cuales se puede elegir una victima.
bool (*asignacion)(int32_t *start, int32_t *end, uint32_t PID);

typedef struct heapMetadata { 
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isFree;
 } t_heapMetadata;

typedef struct mem {
    void *memory;
    t_memoryConfig *config;
} t_memory;

typedef struct frameMetadata {
    bool isFree;
    bool modified;
    bool u;         // Para el clock-m
    uint32_t PID;
    uint32_t page;
    uint32_t timeStamp;
} t_frameMetadata;

typedef struct memoryMetadata{
    uint32_t entryQty;
    uint32_t *firstFrame; // Array de PIDS donde el indice es el numero de "bloque" asignado a un carpincho en asig fija.
    uint32_t counter;
    t_frameMetadata *entries;
} t_memoryMetadata;

t_memory *ram;
t_memoryMetadata *metadata;

t_memory *initializeMemory(t_memoryConfig *config);
void destroyMemory(t_memory* mem);

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config);
void destroyMemoryMetadata(t_memoryMetadata *metadata);

// Utils
int32_t getPage(uint32_t address, t_memoryConfig *cfg);
int32_t getOffset(uint32_t address, t_memoryConfig *cfg);
bool isFree(uint32_t frame);
uint32_t getFrameTimestamp(uint32_t frame);


// Asignacion:
bool fijo(int32_t *start, int32_t *end, uint32_t PID);
bool global(int32_t *start, int32_t *end, uint32_t PID); // Param PID no se usa, pero es necesario para cumplir con interfaz.

/**
 * @DESC: Devuelve un puntero al frame pedido.
 * @param ram: memoria.
 * @param frameN: numero de frame requerido.
 * @return void*: puntero al frame requerido.
 */
void *ram_getFrame(t_memory* ram, uint32_t frameN);
void ram_editFrame(t_memory *mem, uint32_t offset, uint32_t frame, void *from, uint32_t size);
void readFrame(t_memory *mem, uint32_t frame, void *dest);
void writeFrame(t_memory *mem, uint32_t frame, void *from);


/**
 * @DESC: trae una pagina de swap realizando los reemplazos correspondientes.
 * @param PID: PID del procesos solicitante.
 * @param pageN: pagina solicitada.
 * @return uint32_t: numero de frame en el que quedo la pagina.
 */
uint32_t swapPage(uint32_t PID, uint32_t pageN);
int32_t getFreeFrame(int32_t start, int32_t end);
int32_t getFrame(uint32_t PID, uint32_t page);
uint32_t replace(uint32_t victim, uint32_t PID, uint32_t page);

// TODO: Implementar algorimo clock-m:
uint32_t clock_m(int32_t start, int32_t end);
uint32_t LRU(int32_t start, int32_t end);

#endif // !MEMORY_H_

