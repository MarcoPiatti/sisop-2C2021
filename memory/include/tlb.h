#ifndef TLB_H_
#define TLB_H_

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>

typedef struct TLBEntry {
    uint32_t pid;
    int32_t page;
    int32_t frame;
    bool used;
} t_TLBEntry;

typedef struct TLB t_TLB;

/**
 * @DESC: TAD para la TLB
 * -size: cantidad de entradas de la TLB
 * -table: la tabla en si, representada como un array
 * -counts: lista con todos los hits y misses, usada para metricas
 * -victims: cola para gestionar a que entrada reemplazar
 * -algorithm: algoritmo de reemplazo de entradas
 */
struct TLB {
    int size;
    t_TLBEntry *table;
    t_list* counts;
    t_queue* victims;
    void (*algorithm)(t_TLB* self, t_TLBEntry* entry);
    pthread_mutex_t mutex;
};

void fifo(t_TLB* self, t_TLBEntry* entry);

void lru(t_TLB* self, t_TLBEntry* entry);

//TODO: pasarle los delays a la TLB

/**
 * @DESC: Crea una TLB
 * @param size: cantidad de entradas asignadas
 * @param algorithm: algoritmo de reemplazo a utilizar (fifo/lru)
 * @return t_TLB*: la nueva TLB
 */
t_TLB* createTLB(int size, void (*algorithm)(t_TLB* self, t_TLBEntry* entry));

/**
 * @DESC: Destruye una TLB
 */
void destroyTLB(t_TLB* self);

/**
 * @DESC: Dados un PID y un nro. de Pagina, devuelve un nro. de Frame o -1.
 * @param self: la TLB a utilizar
 * @param pid: ID del proceso que necesita el frame
 * @param page: Nro. de Pagina correspondiente al frame
 * @return int32_t: Nro. de Frame conseguido, o -1 si no habia.
 */
int32_t TLB_findFrame(t_TLB* self, uint32_t pid, int32_t page);

/**
 * @DESC: Agrega una nueva entrada a la TLB, reemplaza a una victima si no hay lugar
 * @param self: la TLB a utilizar
 * @param pid: ID del proceso que tiene el frame
 * @param page: Nro. de Pagina correspondiente al frame
 * @param frame: Frame de la Ram
 */
void TLB_addEntry(t_TLB* self, uint32_t pid, int32_t page, int32_t frame);

/**
 * @DESC: Despeja una entrada de la tlb, si es que se encuentra, sino no pasa nada
 * @param pid: PID de la entrada
 * @param page: Pagina de la entrada
 * @param frame: Frame de la entrada
 */
void TLB_clearIfExists(t_TLB* self, uint32_t pid, int32_t page, int32_t frame);

/**
 * @DESC: Limpia todas las entradas de una TLB, dejandolas vacias
 * @param self: TLB a limpiar
 */
void TLB_clear(t_TLB* self);

/**
 * @DESC: Crea un archivo con info sobre una TLB en un directorio dado
 * @param self: TLB a mostrar
 * @param dumpDir: Directorio donde se generara el nuevo archivo
 */
void TLB_dump(t_TLB* self, char* dumpDir);

/**
 * @DESC: Muestra por pantalla informacion sobre los hits y misses de una TLB
 * @param self: TLB a mostrar
 */
void TLB_printCounts(t_TLB* self);

#endif // !TLB_H_
