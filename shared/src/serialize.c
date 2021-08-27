#include "serialize.h"

#define INITIAL_STREAM_SIZE 256

streamBuffer* createStream_S(){
    streamBuffer* tmp = malloc(sizeof(streamBuffer));
    tmp->offset = 0;
    tmp->mallocSize = INITIAL_STREAM_SIZE;
    tmp->stream = malloc(tmp->mallocSize);
    return tmp;
}

streamBuffer* createStream_D(size_t size){
    streamBuffer* tmp = malloc(sizeof(streamBuffer));
    tmp->offset = 0;
    tmp->mallocSize = size;
    tmp->stream = malloc(size);
    return tmp;
}

void destroyStream(streamBuffer* stream){
    free(stream->stream);
    free(stream);
}

void streamAssertFits(streamBuffer* stream, size_t size){
    while(stream->mallocSize < stream->offset + size){
        stream->mallocSize += INITIAL_STREAM_SIZE;
        stream->stream = realloc(stream->stream, stream->mallocSize);
    }
}

void streamAdd(streamBuffer* stream, void* source, size_t size){
    streamAssertFits(stream, size);
    memcpy(stream->stream + stream->offset, source, size);
    stream->offset += size;
}

void streamAdd_INT32(streamBuffer* stream, void* source){
    streamAdd(stream, source, sizeof(int32_t));
}

void streamAdd_INT32(streamBuffer* stream, int32_t value){
    int32_t tmp = value;
    streamAdd_INT32(stream, (void*)&tmp);
}

void streamAdd_UINT32(streamBuffer* stream, void* source){
    streamAdd(stream, source, sizeof(uint32_t));
}

void streamAdd_UINT8(streamBuffer* stream, void* source){
    streamAdd(stream, source, sizeof(uint8_t));
}

void streamAdd_STRING(streamBuffer* stream, void* source){
    uint32_t size = string_length(source) + 1;
    streamAdd_UINT32(stream, &size);
    streamAdd(stream, source, size);
}

void streamAdd_LIST(streamBuffer* stream, t_list* source, void(*streamAdd_ELEM)(streamBuffer*, void*)){
    void _streamAdd_ELEM(void* elem){
        streamAdd_ELEM(stream, elem);
    };
    uint32_t listSize = source->elements_count;
    streamAdd_UINT32(stream, &listSize);
    list_iterate(source, _streamAdd_ELEM);
}

void streamAdd_DICT(streamBuffer* stream, t_dictionary* source, void(*streamAdd_VALUE_P)(streamBuffer*, void*)){
    void _streamADD_KEYVALUE(char* key, void* value){
        streamAdd_STRING(stream, key);
        streamAdd_VALUE_P(stream, value);
    };
    uint32_t dictionarySize = dictionary_size(source);
    streamAdd_UINT32(stream, &dictionarySize);
    dictionary_iterator(source, _streamADD_KEYVALUE);
}

void streamTake(streamBuffer* stream, void** dest, size_t size){
    guard_nullPtr(dest, size);
    memcpy(dest, stream->stream + stream->offset, size);
    stream->offset += size;
}

void streamTake_INT32_P(streamBuffer* stream, void** dest){
    streamTake(stream, dest, sizeof(int32_t));
}

int32_t streamTake_INT32(streamBuffer* stream){
    int32_t tmp;
    int32_t* tmpPtr = &tmp;
    streamTake_INT32_P(stream, (void**)&tmpPtr);
    return tmp;
}

void streamTake_UINT32_P(streamBuffer* stream, void** dest){
    streamTake(stream, dest, sizeof(uint32_t));
}

uint32_t streamTake_UINT32(streamBuffer* stream){
    uint32_t tmp;
    uint32_t* tmpPtr = &tmp;
    streamTake_UINT32_P(stream, (void**)&tmpPtr);
    return tmp;
}

void streamTake_UINT8_P(streamBuffer* stream, void** dest){
    streamTake(stream, dest, sizeof(uint8_t));
}

uint8_t streamTake_UINT8(streamBuffer* stream){
    uint8_t tmp;
    uint8_t* tmpPtr = &tmp;
    streamTake_UINT8_P(stream, (void**)&tmpPtr);
    return tmp;
}

void streamTake_STRING_P(streamBuffer* stream, void** dest){
    uint32_t size = streamTake_UINT32(stream);
    streamTake(stream, dest, size);
}

char* streamTake_STRING(streamBuffer* stream){
    char* tmp;
    streamTake_STRING_P(stream, (void**)&tmp);
    return tmp;
}

void streamTake_LIST_P(streamBuffer* stream, t_list** source, void(*streamTake_ELEM_P)(streamBuffer*, void**)){
    guard_nullList(source);
    uint32_t listSize = streamTake_UINT32(stream);
    for (uint32_t i = 0; i < listSize; i++){
        void* elem;
        streamTake_ELEM_P(stream, &elem);
        list_add(*source, elem);
    }
}

t_list* streamTake_LIST(streamBuffer* stream, void(*streamTake_ELEM_P)(streamBuffer*, void**)){
    t_list* tmpList;
    streamTake_LIST_P(stream, &tmpList, streamTake_ELEM_P);
    return tmpList;
}

void streamTake_DICT_P(streamBuffer* stream, t_dictionary** source, void(*streamTake_VALUE_P)(streamBuffer*, void**)){
    guard_nullDict(source);
    uint32_t dictSize = streamTake_UINT32(stream);
    for(uint32_t i = 0; i < dictSize; i++){
        char* tmpKey = streamTake_STRING(stream);
        void* tmpValue; 
        streamTake_VALUE_P(stream, &tmpValue);
        dictionary_put(*source, tmpKey, tmpValue);
    }
}

t_dictionary* streamTake_DICT(streamBuffer* stream, void(*streamTake_VALUE_P)(streamBuffer*, void**)){
    t_dictionary* tmpDict;
    streamTake_DICT_P(stream, &tmpDict, streamTake_VALUE_P);
    return tmpDict;
}