#ifndef MEMORY_H_
#define MEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include "memoryConfig.h"
#include "networking.h"
#include "swapInterface.h"
#include "pageTable.h"

t_swapInterface* swapInterface;
void* auxHandler(void *vclientSocket);

// Algoritmo (clock-m o LRU) toma un frame de inicio y un frame final y eligen la victima dentro del rango.
uint32_t (*algoritmo)(uint32_t start, uint32_t end);

// Asignacion fija o global, devuelven en los parametros un rango de frames entre los cuales se puede elegir una victima.
void (*assignacion)(uint32_t *start, uint32_t *end, uint32_t PID);

// TODO: Chequear que la logica de memwrite sea correcta.

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
    uint32_t PID;
    uint32_t page;
    uint32_t timeStamp;     // TODO: Cuando actualizar timestamp;
} t_frameMetadata;

typedef struct memoryMetadata{
    uint32_t entryQty;
    t_frameMetadata **entries;
} t_memoryMetadata;

extern bool (*petitionHandlers[MAX_PETITIONS])(t_packet* petition, int socket);

t_log *logger;
t_memory *ram;
t_dictionary *pageTables;
t_memoryConfig *config;
t_memoryMetadata *metadata;


t_memory *initializeMemory(t_memoryConfig *config);

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config);

void *petitionHandler(void *_clientSocket);

void destroyMemoryMetadata(t_memoryMetadata *metadata);

void memread(uint32_t bytes, uint32_t address, int PID, void *destination);

void memwrite(uint32_t bytes, uint32_t address, int PID, void *from);

int32_t getFreeFrame(uint32_t start, uint32_t end);

int32_t getPage(uint32_t address, t_memoryConfig *cfg);

int32_t getOffset(uint32_t address, t_memoryConfig *cfg);

int32_t getFrame(uint32_t PID, uint32_t page);

// Asignacion:
void fijo(int32_t *start, int32_t *end, uint32_t PID);
void global(int32_t *start, int32_t *end, uint32_t PID);

/**
 * @DESC: Devuelve un puntero al frame pedido.
 * @param ram: memoria.
 * @param frameN: numero de frame requerido.
 * @return void*: puntero al frame requerido.
 */
void *ram_getFrame(t_memory* ram, uint32_t frameN);

uint32_t replace(uint32_t victim, uint32_t PID, uint32_t page);

bool isFree(uint32_t frame)
uint32_t getFrameTimestamp(uint32_t frame);

uint32_t LRU(uint32_t start, uint32_t end);

void ram_editFrame(t_memory *mem, uint32_t offset, uint32_t frame, void *from, uint32_t size);

/**
 * @DESC: trae una pagina de swap realizando los reemplazos correspondientes. A IMPLEMENTAR.
 * @param PID: PID del procesos solicitante.
 * @param pageN: pagina solicitada.
 * @return uint32_t: numero de frame en el que quedo la pagina.
 */
uint32_t swapPage(uint32_t PID, uint32_t pageN);

// TODO: Implementar algorimos clock-m:
uint32_t clock_m(uint32_t start, uint32_t end);
uint32_t LRU(uint32_t start, uint32_t end);



#endif // !MEMORY_H_

