#ifndef SWAPFILE_H_
#define SWAPFILE_H_

#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <commons/collections/list.h>

typedef struct pageMetadata{
    uint32_t pid;
    int32_t pageNumber;
    bool used;
} t_pageMetadata;

/**
 * @DESC: TAD para un Archivo de Swap
 * - path: al archivo real en filesystem
 * - fd: file descriptor del archivo
 * - size: tamanio total en bytes del archivo
 * - pageSize: tamanio en bytes de cada pagina del archivo
 * - maxPages: cantidad de paginas que entran en el archivo
 * - entries: array con info sobre cada pagina guardada
 */
typedef struct swapFile{
    char* path;
    int fd;
    size_t size;
    size_t pageSize;
    int maxPages;
    t_pageMetadata* entries;
} t_swapFile;

/**
 * @DESC: Crea un swapFile a partir de un path, un tamanio total y un tamanio de pagina
 */
t_swapFile* swapFile_create(char* path, size_t size, size_t pageSize);

/**
 * @DESC: Destruye un swapFile de memoria
 */
void swapFile_destroy(t_swapFile* self);

/**
 * @DESC: Llena de 0s la pagina indicada por index
 */
void swapFile_clearAtIndex(t_swapFile* sf, int index);

/**
 * @DESC: Lee los contenidos de una pagina indicada por index
 * @param sf: Swapfile leido
 * @param index: indice a leer
 * @return void*: puntero a los datos leidos de la pagina
 */
void* swapFile_readAtIndex(t_swapFile* sf, int index);

/**
 * @DESC: Sobreescribe los contenidos de una pagina con los dados
 * @param sf: Swapfile a escribir
 * @param index: indice a escribir
 * @param pagePtr: puntero a los nuevos datos
 */
void swapFile_writeAtIndex(t_swapFile* sf, int index, void* pagePtr);

bool swapFile_isFull(t_swapFile* sf);

bool swapFile_hasRoom(t_swapFile* sf);

/**
 * @DESC: Retorna true si hay alguna entrada del swapFile usada por el PID dado
 */
bool swapFile_hasPid(t_swapFile* sf, uint32_t pid);

/**
 * @DESC: Cuenta la cantidad de paginas que un PID tiene en un swapfile
 */
int swapFile_countPidPages(t_swapFile* sf, uint32_t pid);

/**
 * @DESC: Revisa si un indice de un swapfile esta marcado como disponible
 */
bool swapFile_isFreeIndex(t_swapFile* sf, int index);

/**
 * @DESC: A partir de un pid y nro. de pagina, obtiene el indice donde se encuentran los datos de la pagina
 */
int swapFile_getIndex(t_swapFile* sf, uint32_t pid, int32_t pageNumber);

/**
 * @DESC: Busca y retorna el primer indice disponible, retorna -1 si no lo hay
 */
int swapFile_findFreeIndex(t_swapFile* sf);

/**
 * @DESC: Sobreescribe la metadata de un indice dado, marcandolo como usado
 * @param sf: Swapfile usado
 * @param pid: PID que usa ese indice
 * @param pageNumber: Pagina a la que corresponde el contenido guardado en el indice
 * @param index: Indice a reescribir metadata
 */
void swapFile_register(t_swapFile* sf, uint32_t pid, int32_t pageNumber, int index);

#endif // !SWAPFILE_H_
