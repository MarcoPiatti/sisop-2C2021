#ifndef SWAP_H_
#define SWAP_H_

#include "swapConfig.h"
#include <commons/collections/list.h>
#include <stdint.h>
#include "networking.h"
#include "swapFile.h"
#include "logs.h"

t_swapConfig* swapConfig;

// Lista de archivos de swap usados
t_list* swapFiles;

/**
 * @DESC: Revisa si un PID existe entre todos los archivos del swap
 * @param pid: PID buscado
 * @return t_swapFile*: puntero al swapFile que tiene al PID, NULL si no hay ninguno
 */
t_swapFile* pidExists(uint32_t pid);

/**
 * @DESC: Puntero a funcion usado para elegir el modo de asignacion (fija/global)
 */
bool (*asignacion)(uint32_t pid, int32_t page, void* pageContent);

/**
 * @DESC: Funciones con respuestas a peticiones de memoria, definidas en petitionHandlers.c
 */
extern void (*petitionHandler[MAX_MEM_PETITIONS])(t_packet* petition, int memorySocket);

#endif // !SWAP_H_