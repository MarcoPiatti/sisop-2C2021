#ifndef GUARDS_H_
#define GUARDS_H_

#include "logs.h"
#include <commons/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <errno.h>

/**
 * @DESC: revisa si un puntero es nulo y le asigna memoria
 * @param ptr: puntero a proteger pasado por referencia
 * @param size: tamanio de memoria a asignarle
 */
void guard_nullPtr(void** ptr, size_t size);

/**
 * @DESC: revisa si un puntero a lista apunta a null y llama a list_create()
 * @param list: puntero a la lista a proteger, por referencia
 */
void guard_nullList(t_list** list);

/**
 * @DESC: revisa si un puntero a diccionario apunta a null y llama a dict_create()
 * @param dict: puntero al diccionario a proteger, por referencia
 */
void guard_nullDict(t_dictionary** dict);

/**
 * @DESC: 
 * @param returnCode:
 */
void guard_syscall(int returnCode);

#endif // !GUARDS_H_
