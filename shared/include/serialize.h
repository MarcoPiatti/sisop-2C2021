#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include "guards.h"
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

#define INITIAL_STREAM_SIZE 256

/**
 * @DESC: Contiene 
 *        - un stream
 *        - el tamanio de su informacion
 *        - su tamanio alojado en memoria
 */
typedef struct t_streamBuffer {
    uint32_t offset;
    size_t mallocSize;
    char* stream;
} t_streamBuffer;

/**
 * @DESC: Crea un objeto Stream en memoria para deserializar
 *        - el stream se aloca con tamanio "size"
 *        - el offset se inicializa en 0
 * @return streamBuffer*: puntero al stream creado
 */
t_streamBuffer* createStream(size_t size);

/**
 * @DESC: Destruye un streamBuffer, y su stream, de la memoria
 * @param stream: puntero al streamBuffer a destruir
 */
void destroyStream(t_streamBuffer* stream);

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega al stream "size" bytes desde "source"
 * @param stream: puntero al stream al que se agrega el contenido
 * @param source: puntero al inicio del contenido
 * @param size: tamanio de bytes a agregar
 */
void streamAdd(t_streamBuffer* stream, void* source, size_t size);

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega un int32_t al stream
 * @param stream: puntero al stream
 * @param source: puntero al int32_t
 */
void streamAdd_INT32_P(t_streamBuffer* stream, void* source);

/**
 * @DESC: Agrega un int32_t al stream
 * @param stream: puntero al stream
 * @param source: valor del int32_t
 */
void streamAdd_INT32(t_streamBuffer* stream, int32_t source);

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega un uint32_t al stream
 * @param stream: puntero al stream
 * @param source: puntero al uint32_t
 */
void streamAdd_UINT32_P(t_streamBuffer* stream, void* source);

/**
 * @DESC: Agrega un uint32_t al stream
 * @param stream: puntero al stream
 * @param source: valor del uint32_t
 */
void streamAdd_UINT32(t_streamBuffer* stream, uint32_t source);

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega un uint8_t al stream
 * @param stream: puntero al stream
 * @param source: puntero al uint8_t
 */
void streamAdd_UINT8_P(t_streamBuffer* stream, void* source);

/**
 * @DESC: Agrega un uint8_t al stream
 * @param stream: puntero al stream
 * @param source: valor del uint8_t
 */
void streamAdd_UINT8(t_streamBuffer* stream, uint8_t source);

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega un string (y su tamanio antes) al stream
 * @param stream: puntero al stream
 * @param source: puntero al string (castear a void*)
 */
void streamAdd_STRING_P(t_streamBuffer* stream, void* source);

/**
 * @DESC: Agrega un string (y su tamanio antes) al stream
 * @param stream: puntero al stream
 * @param source: puntero al string
 */
void streamAdd_STRING(t_streamBuffer* stream, char* source);

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega la cantidad de elementos de una lista, y luego todos los elementos, al stream
 * @param stream: puntero al stream
 * @param source: puntero a la lista
 * @param streamAdd_ELEM: funcion para agregar un elemento al stream
 */
void streamAdd_LIST(t_streamBuffer* stream, t_list* source, void(*streamAdd_ELEM)(t_streamBuffer*, void*));

//----------------------------------------------------------------------//

/**
 * @DESC: Agrega la cantidad de elementos en el diccionario, seguido de todos los pares KEY/VALUE
 * @param stream: puntero al stream
 * @param source: puntero al diccionario
 * @param streamAdd_VALUE: funcion para agregar un value al stream (el key se agrega solo)
 */
void streamAdd_DICT(t_streamBuffer* stream, t_dictionary* source, void(*streamAdd_VALUE)(t_streamBuffer*, void*));

//----------------------------------------------------------------------//

/**
 * @DESC: Toma "size" bytes del stream y los guarda en memoria a partir de "dest"
 * @param stream: puntero al stream
 * @param dest: puntero destino pasado por referencia
 * @param size: cantidad de bytes a sacar del stream
 */
void streamTake(t_streamBuffer* stream, void** dest, size_t size);

//----------------------------------------------------------------------//

/**
 * @DESC: Toma un int32_t del stream y lo copia a dest
 * @param stream: puntero al stream
 * @param dest: puntero a int32_t pasado por referencia
 */
void streamTake_INT32_P(t_streamBuffer* stream, void** dest);

/**
 * @DESC: Retorna por valor un int32_t del stream
 * @param stream: puntero al stream
 * @return int32_t: valor del int32_t recuperado
 */
int32_t streamTake_INT32(t_streamBuffer* stream);

//----------------------------------------------------------------------//

/**
 * @DESC: Toma un uint32_t del stream y lo copia a dest
 * @param stream: puntero al stream
 * @param dest: puntero a uint32_t pasado por referencia
 */
void streamTake_UINT32_P(t_streamBuffer* stream, void** dest);

/**
 * @DESC: Retorna por valor un uint32_t del stream
 * @param stream: puntero al stream
 * @return int32_t: valor del uint32_t recuperado
 */
uint32_t streamTake_UINT32(t_streamBuffer* stream);

//----------------------------------------------------------------------//

/**
 * @DESC: Toma un uint8_t del stream y lo copia a dest
 * @param stream: puntero al stream
 * @param dest: puntero a uint8_t pasado por referencia
 */
void streamTake_UINT8_P(t_streamBuffer* stream, void** dest);

/**
 * @DESC: Retorna por valor un uint8_t del stream
 * @param stream: puntero al stream
 * @return int32_t: valor del uint8_t recuperado
 */
uint8_t streamTake_UINT8(t_streamBuffer* stream);

//----------------------------------------------------------------------//

/**
 * @DESC: Toma un string el stream (y previamente su tamanio) y lo copia a dest
 * @param stream: puntero al stream
 * @param dest: puntero al string (char*) pasado por referencia
 */
void streamTake_STRING_P(t_streamBuffer* stream, void** dest);

/**
 * @DESC: Retorna "por valor" un string (char*) del stream
 * @param stream: puntero al stream
 * @return char*: puntero al string alojado en memoria
 */
char* streamTake_STRING(t_streamBuffer* stream);

//----------------------------------------------------------------------//

/**
 * @DESC: Toma del stream la cantidad de elementos de una lista, y luego todos sus elementos, y los agrega a source
 * @param stream: puntero al stream
 * @param source: puntero a la lista pasado por referencia
 * @param streamTake_ELEM: funcion para tomar un elemento del stream
 */
void streamTake_LIST_P(t_streamBuffer* stream, t_list** source, void(*streamTake_ELEM_P)(t_streamBuffer*, void**));

/**
 * @DESC: Toma del stream una lista y retorna un puntero a esa misma
 * @param stream: puntero al stream
 * @param streamTake_ELEM_P: funcion para tomar un elemento del stream
 * @return t_list*: puntero a la lista deserializada
 */
t_list* streamTake_LIST(t_streamBuffer* stream, void(*streamTake_ELEM_P)(t_streamBuffer*, void**));

//----------------------------------------------------------------------//

/**
 * @DESC: Toma del stream la cantidad de pares key/value del diccionario, y luego todos los pares key/value (en ese orden)
 *        y los agrega a source
 * @param stream: puntero al stream
 * @param source: puntero al diccionario pasado por referencia
 * @param streamTake_VALUE: funcion para tomar un value del stream (el key se recupera solo)
 */
void streamTake_DICT_P(t_streamBuffer* stream, t_dictionary** source, void(*streamTake_VALUE_P)(t_streamBuffer*, void**));

/**
 * @DESC: Toma del stream un diccionario y retorna un puntero al mismo
 * @param stream: puntero al stream
 * @param streamTake_VALUE_P: funcion para tomar un value del stream (el key se recupera solo)
 * @return t_dictionary*: puntero al diccionario deserializado
 */
t_dictionary* streamTake_DICT(t_streamBuffer* stream, void(*streamTake_VALUE_P)(t_streamBuffer*, void**));

#endif // !SERIALIZE_H_