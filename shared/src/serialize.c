#include "serialize.h"

t_streamBuffer* createStream(size_t size){
    t_streamBuffer* tmp = malloc(sizeof(t_streamBuffer));
    tmp->offset = 0;
    tmp->mallocSize = size;
    tmp->stream = malloc(size);
    return tmp;
}

void destroyStream(t_streamBuffer* stream){
    free(stream->stream);
    free(stream);
}

//----------------------------------------------------------------------//

void streamAssertFits(t_streamBuffer* stream, size_t size){
    while(stream->mallocSize < stream->offset + size){
        stream->mallocSize += INITIAL_STREAM_SIZE;
        stream->stream = realloc(stream->stream, stream->mallocSize);
    }
}

//----------------------------------------------------------------------//

void streamAdd(t_streamBuffer* stream, void* source, size_t size){
    streamAssertFits(stream, size);
    memcpy(stream->stream + stream->offset, source, size);
    stream->offset += size;
}

//----------------------------------------------------------------------//

void streamAdd_INT32_P(t_streamBuffer* stream, void* source){
    streamAdd(stream, source, sizeof(int32_t));
}

void streamAdd_INT32(t_streamBuffer* stream, int32_t value){
    streamAdd_INT32_P(stream, (void*)&value);
}

//----------------------------------------------------------------------//

void streamAdd_UINT32_P(t_streamBuffer* stream, void* source){
    streamAdd(stream, source, sizeof(uint32_t));
}

void streamAdd_UINT32(t_streamBuffer* stream, uint32_t value){
    streamAdd_UINT32_P(stream, (void*)&value);
}

//----------------------------------------------------------------------//

void streamAdd_UINT8_P(t_streamBuffer* stream, void* source){
    streamAdd(stream, source, sizeof(uint8_t));
}

void streamAdd_UINT8(t_streamBuffer* stream, uint8_t value){
    streamAdd_UINT8_P(stream, (void*)&value);
}

//----------------------------------------------------------------------//

void streamAdd_STRING_P(t_streamBuffer* stream, void* source){
    uint32_t size = string_length((char*)source) + 1;
    streamAdd_UINT32(stream, size);
    streamAdd(stream, source, size);
}

void streamAdd_STRING(t_streamBuffer* stream, char* source){
    streamAdd_STRING_P(stream, (void*)source);
}

//----------------------------------------------------------------------//

void streamAdd_LIST(t_streamBuffer* stream, t_list* source, void(*streamAdd_ELEM_P)(t_streamBuffer*, void*)){
    void _streamAdd_ELEM_P(void* elem){
        streamAdd_ELEM_P(stream, elem);
    };
    uint32_t listSize = source->elements_count;
    streamAdd_UINT32(stream, listSize);
    list_iterate(source, _streamAdd_ELEM_P);
}

//----------------------------------------------------------------------//

void streamAdd_DICT(t_streamBuffer* stream, t_dictionary* source, void(*streamAdd_VALUE_P)(t_streamBuffer*, void*)){
    void _streamADD_KEYVALUE_P(char* key, void* value){
        streamAdd_STRING(stream, key);
        streamAdd_VALUE_P(stream, value);
    };
    uint32_t dictionarySize = dictionary_size(source);
    streamAdd_UINT32(stream, dictionarySize);
    dictionary_iterator(source, _streamADD_KEYVALUE_P);
}

//----------------------------------------------------------------------//

void streamTake(t_streamBuffer* stream, void** dest, size_t size){
    guard_nullPtr(dest, size);
    memcpy(*dest, stream->stream + stream->offset, size);
    stream->offset += size;
}

//----------------------------------------------------------------------//

void streamTake_INT32_P(t_streamBuffer* stream, void** dest){
    streamTake(stream, dest, sizeof(int32_t));
}

int32_t streamTake_INT32(t_streamBuffer* stream){
    int32_t tmp;
    int32_t* tmpPtr = &tmp;
    streamTake_INT32_P(stream, (void**)&tmpPtr);
    return tmp;
}

//----------------------------------------------------------------------//

void streamTake_UINT32_P(t_streamBuffer* stream, void** dest){
    streamTake(stream, dest, sizeof(uint32_t));
}

uint32_t streamTake_UINT32(t_streamBuffer* stream){
    uint32_t tmp;
    uint32_t* tmpPtr = &tmp;
    streamTake_UINT32_P(stream, (void**)&tmpPtr);
    return tmp;
}

//----------------------------------------------------------------------//

void streamTake_UINT8_P(t_streamBuffer* stream, void** dest){
    streamTake(stream, dest, sizeof(uint8_t));
}

uint8_t streamTake_UINT8(t_streamBuffer* stream){
    uint8_t tmp;
    uint8_t* tmpPtr = &tmp;
    streamTake_UINT8_P(stream, (void**)&tmpPtr);
    return tmp;
}

//----------------------------------------------------------------------//

void streamTake_STRING_P(t_streamBuffer* stream, void** dest){
    uint32_t size = streamTake_UINT32(stream);
    streamTake(stream, dest, size);
}

char* streamTake_STRING(t_streamBuffer* stream){
    char* tmp = NULL;
    streamTake_STRING_P(stream, (void**)&tmp);
    return tmp;
}

//----------------------------------------------------------------------//

void streamTake_LIST_P(t_streamBuffer* stream, t_list** source, void(*streamTake_ELEM_P)(t_streamBuffer*, void**)){
    guard_nullList(source);
    uint32_t listSize = streamTake_UINT32(stream);
    for (uint32_t i = 0; i < listSize; i++){
        void* elem = NULL;
        streamTake_ELEM_P(stream, &elem);
        list_add(*source, elem);
    }
}

t_list* streamTake_LIST(t_streamBuffer* stream, void(*streamTake_ELEM_P)(t_streamBuffer*, void**)){
    t_list* tmpList = NULL;
    streamTake_LIST_P(stream, &tmpList, streamTake_ELEM_P);
    return tmpList;
}

//----------------------------------------------------------------------//

void streamTake_DICT_P(t_streamBuffer* stream, t_dictionary** source, void(*streamTake_VALUE_P)(t_streamBuffer*, void**)){
    guard_nullDict(source);
    uint32_t dictSize = streamTake_UINT32(stream);
    for(uint32_t i = 0; i < dictSize; i++){
        char* tmpKey = streamTake_STRING(stream);
        void* tmpValue = NULL; 
        streamTake_VALUE_P(stream, &tmpValue);
        dictionary_put(*source, tmpKey, tmpValue);
    }
}

t_dictionary* streamTake_DICT(t_streamBuffer* stream, void(*streamTake_VALUE_P)(t_streamBuffer*, void**)){
    t_dictionary* tmpDict = NULL;
    streamTake_DICT_P(stream, &tmpDict, streamTake_VALUE_P);
    return tmpDict;
}