#include "guards.h"

void guard_nullPtr(void** ptr, size_t size){
    if(*ptr == NULL) *ptr = calloc(1, size);
}

void guard_nullList(t_list** list){
    if(*list == NULL) *list = list_create();
}

void guard_nullDict(t_dictionary** dict){
    if(*dict == NULL) *dict = dictionary_create();
}