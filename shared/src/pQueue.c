#include "pQueue.h"

t_pQueue* pQueue_create(){
    t_pQueue* queue = malloc(sizeof(t_pQueue));
    queue->elems = queue_create();
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->sem, 0, 0);
    return queue;
}

void pQueue_destroy(t_pQueue* queue, void(*elemDestroyer)(void*)){
    pthread_mutex_lock(&queue->mutex);
    queue_destroy_and_destroy_elements(queue->elems, elemDestroyer);
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    sem_destroy(&queue->sem);
    free(queue);
}

void pQueue_put(t_pQueue* queue, void* elem){
    pthread_mutex_lock(&queue->mutex);
    queue_push(queue->elems, (void*)elem);
    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem);
}

void* pQueue_take(t_pQueue* queue){
    sem_wait(&queue->sem);
    pthread_mutex_lock(&queue->mutex);
    void* elem = queue_pop(queue->elems);
    pthread_mutex_unlock(&queue->mutex);
    return elem;
}

bool pQueue_isEmpty(t_pQueue* queue){
    pthread_mutex_lock(&queue->mutex);
    int result = queue_is_empty(queue->elems);
    pthread_mutex_unlock(&queue->mutex);
    result;
}

void pQueue_sort(t_pQueue* queue, bool (*algorithm)(void*, void*)){
    pthread_mutex_lock(&queue->mutex);
    list_sort(queue->elems->elements, algorithm);
    pthread_mutex_unlock(&queue->mutex);
}