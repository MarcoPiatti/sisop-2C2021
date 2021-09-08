#include "kernel.h"

t_process* createProcess(int id){
    t_process* process = malloc(sizeof(t_process));
    process->tasks = queue_create();
    process->pid = id;
    return process;
}

void destroyProcess(t_process* process){
    queue_destroy_and_destroy_elements(process->tasks, free);
    free(process);
}

t_processList* createProcessQueue(){
    t_processList* queue = malloc(sizeof(t_processList));
    queue->elems = queue_create();
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->sem, 0, 0);
    return queue;
}

void destroyProcessQueue(t_processList* queue){
    void _destroyProcess(void* process){
        destroyProcess((t_process*)process);
    };
    queue_destroy_and_destroy_elements(queue->elems, _destroyProcess);
    pthread_mutex_destroy(&queue->mutex);
    sem_destroy(&queue->sem);
    free(queue);
}

void putProcess(t_process* process, t_processList* queue){
    pthread_mutex_lock(&queue->mutex);
    queue_push(queue->elems, (void*)process);
    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem);
}

t_process* takeProcess(t_processList* queue){
    sem_wait(&queue->sem);
    pthread_mutex_lock(&queue->mutex);
    t_process* process = queue_pop(queue->elems);
    pthread_mutex_unlock(&queue->mutex);
    return process;
}

void* processInitializer(void* nada){
    while(1){
        t_process* process = takeProcess(new);
        //Realiza algun procesamiento...
        putProcess(process, ready);
    }
}

int runCPU(t_queue* tasks, int quantums){
    t_task* task = queue_peek(tasks);
    if(task == NULL) return -1;
    if(task->isIO) return 0;
    while(quantums){
        sleep(QUANTUM_LENGTH);
        task->remaining--;
        quantums--;
        if(task->remaining == 0){
            task = queue_pop(tasks); 
            free(task);
            task = queue_peek(tasks);
            if(task == NULL) return -1;
            if(task->isIO) return 0;
        }
    }
    return 1;
}

void* executor(void* nada){
    while(1){
        t_process* process = takeProcess(ready);
        int result = runCPU(process->tasks, QUANTUM);
        if(result == 0) putProcess(process, blocked);
        if(result == -1) destroyProcess(process);
        if(result == 1) putProcess(process, ready);
    }   
}

void runIO(t_queue* tasks){
    t_task* task = queue_peek(tasks);
    while(task->isIO){
        sleep(QUANTUM_LENGTH);
        task->remaining--;
        if(task->remaining == 0){
            task = queue_pop(tasks);
            free(task);
            task = queue_peek(tasks);
        }
    }
}

void* executorIO(void* nada){
    while(1){
        t_process* process = takeProcess(blocked);
        runIO(process->tasks);
        putProcess(process, ready);
    }
}

void createScheduler(){
    new = createProcessQueue();
    ready = createProcessQueue();
    blocked = createProcessQueue();
    pthread_create(&thread_processInitializer, NULL, processInitializer, NULL);
    pthread_detach(thread_processInitializer);
    pthread_create(&thread_executorIO, NULL, executorIO, NULL);
    pthread_detach(thread_executorIO);
    for (int i = 0; i < MAX_MULTIPROCESSING; i++){
        pthread_create(&thread_executor[i], NULL, executor, NULL);
        pthread_detach(thread_executor[i]);
    }
}