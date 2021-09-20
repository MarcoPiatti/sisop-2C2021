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

void guard_syscall(int returncode){
    if(returncode == -1){
        int error = errno;
        char* buf = malloc(100);
        strerror_r(error, buf, 100);
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "Error: %s", buf);
        pthread_mutex_unlock(&mutex_log);
        free(buf);
    }
}