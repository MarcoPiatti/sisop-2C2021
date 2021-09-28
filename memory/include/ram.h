#ifndef RAM_H_
#define RAM_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct frameMetadata{
    bool isFree;
    uint32_t pid;
    int32_t page;
    bool modified;
    bool chance;
    int lastUsed;
} t_frameMetadata;

typedef struct ram t_ram;

/**
 * @DESC: TAD para memoria principal
 * - size: tamanio en bytes de la memoria
 * - pageSize: tamanio de las paginas usadas
 * - frames: cantidad de frames totales
 * - data: el puntero a todo el charco de memoria
 * - metadata: array de info de cada frame (pid, pagina, si esta libre, bits de info, etc)
 * - LRU_clock: contador usado en algoritmo LRU.
 * - victimAlgorithm: funcion a usar para elegir un frame victima de un conjunto de frames
 * - assignmentType: funcion a usar para determinar el conjunto de frames que puede usar un PID
 * - maxFrames: maxima cantidad de frames que un PID puede tener en asignacion fija
 */
struct ram{
    int size;
    int pageSize;
    int frames;
    void* data;
    t_frameMetadata *metadata;

    int LRU_clock;
    int(*victimAlgorithm)(t_ram* self, int lowerBound, int upperBound);

    void(*assignmentType)(t_ram* self, uint32_t pid, int* lowerBound, int* upperBound);
    int maxFrames;
};

/**
 * @DESC: Crea un objeto ram en memoria
 * @param size: tamanio en bytes de la memoria principal
 * @param pageSize: tamanio de cada pagina en bytes
 * @param victimAlgorithm: las opciones son LRU o CLOCK_M
 * @param assignmentType: las opciones son fixed o global
 * @param maxFrames: maxima cantidad de frames por proceso segun config
 * @return t_ram*: puntero a la nueva ram creada
 */
t_ram* createRam(int size, int pageSize, int(*victimAlgorithm)(t_ram*, int, int), void(*assignmentType)(t_ram*, uint32_t, int*, int*), int maxFrames);

/**
 * @DESC: Destruye una ram de memoria
 */
void destroyRam(t_ram* self);

/**
 * @DESC: Encuentra el siguiente frame vacio en la ram
 * @param pid: PID del proceso solicitante
 * @return int: Retorna un frame si se encontro uno, -1 si se debe reemplazar algun frame, y -2 si absolutamente no hay cabida para ese PID directamente
 */
int ram_findFreeFrame(t_ram* self, uint32_t pid);

/**
 * @DESC: Retorna la cantidad de frames vacios que puede usar el PID
 * @return int: Retorna -2 Si no hay cabida para ese pid
 */
int ram_countFreeFrames(t_ram* self, uint32_t pid);

/**
 * @DESC: Retorna puntero al frame indicado
 * @param index: indice del frame
 * @return void*: puntero a una copia con los contenidos del frame
 */
void* ram_getFrame(t_ram* self, int index);

/**
 * @DESC: Reescribe los contenidos de un frame dado, con datos de otra pagina
 * @param index: indice del frame
 * @param data: puntero a la data que se copiara al frame
 */
void ram_replaceFrame(t_ram* self, int index, void* data);

/**
 * @DESC: Modifica un frame, no usado para reemplazar, solo editar
 * @param index: indice del frame
 * @param offset: offset donde empezar a escribir
 * @param data: puntero con los datos
 * @param size: cantidad de bytes a escribir
 */
void ram_editFrame(t_ram* self, int index, int offset, void* data, int size);

/**
 * @DESC: Obtiene la metadata de un frame dado
 * @param index: indice del frame
 * @return t_frameMetadata*: puntero a la metadata del frame
 */
t_frameMetadata* ram_getFrameMetadata(t_ram* self, int index);

/**
 * @DESC: Reescribe la metadata de un frame
 * @param index: indice del frame
 * @param pid: nuevo PID del frame
 * @param page: nueva pagina del frame
 */
void ram_replaceFrameMetadata(t_ram* self, int index, uint32_t pid, int32_t page);

/**
 * @DESC: Marca un frame como libre en su metadata
 * @param index: indice del frame
 */
void ram_clearFrameMetadata(t_ram* self, int index);

/**
 * @DESC: Determina un frame victima para reemplazar, usando alguno de los algoritmos. 
 * 
 * NOTA: solo llegar a este punto si findFreeFrame retorno -1.
 * 
 * @param pid: PID del proceso solicitante
 * @return int: indice del frame victima a reemplazar
 */
int ram_getVictimIndex(t_ram* self, uint32_t pid);

/**
 * @DESC: Uno de los dos algoritmos de seleccion de victima, elige a la que menos recientemente se uso.
 * @param lowerBound: primer frame del recorrido
 * @param upperBound: ultimo frame (sin contarlo) del recorrido
 * @return int: indice del frame victima
 */
int LRU(t_ram* self, int lowerBound, int upperBound);

/**
 * @DESC: Uno de los dos algoritmos de seleccion de victima, prioriza los frames que no fueron modificados y que sobrevivieron a una seleccion previa.
 * @param lowerBound: primer frame del recorrido
 * @param upperBound: ultimo frame (sin contarlo) del recorrido
 * @return int: indice del frame victima
 */
int CLOCK_M(t_ram* self, int lowerBound, int upperBound);

/**
 * @DESC: Criterio de asignacion fija. A partir de un PID, retorna el rango de frames a los que puede acceder ese PID.
 * @param pid: PID solicitante
 * @param lowerBound: retornado por referencia, el primer frame usable
 * @param upperBound: retornado por referencia, el ultimo frame (sin contarlo)
 */
void fixed(t_ram* self, uint32_t pid, int* lowerBound, int* upperBound);

/**
 * @DESC: Criterio de asignacion global. el rango es la memoria entera, todos los frames.
 * @param pid: PID solicitante
 * @param lowerBound: retornado por referencia, el primer frame usable
 * @param upperBound: retornado por referencia, el ultimo frame (sin contarlo)
 */
void global(t_ram* self, uint32_t pid, int* lowerBound, int* upperBound);

#endif // !RAM_H_