#include "scheduler.h"

void* processInitializer(void* nada){
    t_process* process;
    while(1){
        sem_wait(&sem_multiprogram);
        if()
            process = takeProcess(new);
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
    new = createpQueue();
    ready = createpQueue();
    blocked = createpQueue();
    pthread_create(&thread_processInitializer, NULL, processInitializer, NULL);
    pthread_detach(thread_processInitializer);
    pthread_create(&thread_executorIO, NULL, executorIO, NULL);
    pthread_detach(thread_executorIO);
    for (int i = 0; i < MAX_MULTIPROCESSING; i++){
        pthread_create(&thread_executor[i], NULL, executor, NULL);
        pthread_detach(thread_executor[i]);
    }
}