#include "kernel.h"

void DDSemInit(t_deadlockDetector* dd, t_packet* newInfo){
    t_mateSem* newSem = (t_mateSem*)streamTake_UINT32(newInfo->payload);
    int newSemValue = streamTake_INT32(newInfo->payload);

    dd->m++;
    dd->sems = realloc(dd->sems, sizeof(t_mateSem*) * dd->m);
    dd->sems[dd->m-1] = newSem;

    dd->available = realloc(dd->available, sizeof(int) * dd->m);
    dd->available[dd->m-1] = newSemValue;
    
    for(int i = 0; i < dd->n; i++){
        dd->allocation[i] = realloc(dd->allocation[i], sizeof(int) * dd->m);
        dd->allocation[i][dd->m-1] = 0;
        dd->request[i] = realloc(dd->request[i], sizeof(int) * dd->m);
        dd->request[i][dd->m-1] = 0;
    }
}

void DDSemAllocatedInstant(t_deadlockDetector* dd, t_packet* newInfo){
    t_process* proc = (t_process*)streamTake_UINT32(newInfo->payload);
    t_mateSem* sem = (t_mateSem*)streamTake_UINT32(newInfo->payload);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->available[j]--;
    dd->allocation[i][j]++;
}

void DDSemAllocated(t_deadlockDetector* dd, t_packet* newInfo){
    t_process* proc = (t_process*)streamTake_UINT32(newInfo->payload);
    t_mateSem* sem = (t_mateSem*)streamTake_UINT32(newInfo->payload);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->available[j]--;
    dd->request[i][j]--;
    dd->allocation[i][j]++;
}

void DDSemRequested(t_deadlockDetector* dd, t_packet* newInfo){
    t_process* proc = (t_process*)streamTake_UINT32(newInfo->payload);
    t_mateSem* sem = (t_mateSem*)streamTake_UINT32(newInfo->payload);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->request[i][j]++;
}

void DDSemRelease(t_deadlockDetector* dd, t_packet* newInfo){
    t_process* proc = (t_process*)streamTake_UINT32(newInfo->payload);
    t_mateSem* sem = (t_mateSem*)streamTake_UINT32(newInfo->payload);

    int j = 0;
    while (dd->sems[j] != sem) j++;

    int i = 0;
    while (dd->procs[i] != proc) i++;

    dd->available[j]++;
    if(dd->allocation[i][j])dd->allocation[i][j]--;
}

void DDSemDestroy(t_deadlockDetector* dd, t_packet* newInfo){
    t_mateSem* sem = (t_mateSem*)streamTake_UINT32(newInfo->payload);

    dd->m--;
    int j = 0;
    while (dd->sems[j] != sem) j++;

    memmove(dd->sems+j, dd->sems+j+1, sizeof(t_mateSem*) * (dd->m - j));
    dd->sems = realloc(dd->sems, sizeof(t_mateSem*) * dd->m);

    memmove(dd->available+j, dd->available+j+1, sizeof(int) * (dd->m - j));
    dd->available = realloc(dd->available, sizeof(int) * dd->m);
    
    for(int i = 0; i < dd->n; i++){
        memmove(dd->allocation[i]+j, dd->allocation[i]+j+1, sizeof(int) * (dd->m - j));
        dd->allocation[i] = realloc(dd->allocation[i], sizeof(int) * dd->m);

        memmove(dd->request[i]+j, dd->request[i]+j+1, sizeof(int) * (dd->m - j));
        dd->request[i] = realloc(dd->request[i], sizeof(int) * dd->m);
    }
}

void DDProcInit(t_deadlockDetector* dd, t_packet* newInfo){
    t_process* newProc = (t_process*)streamTake_UINT32(newInfo->payload);

    dd->n++;
    dd->procs = realloc(dd->procs, sizeof(t_process*) * dd->n);
    dd->procs[dd->n-1] = newProc;

    dd->allocation = realloc(dd->allocation, sizeof(int*) * dd->n);
    dd->request = realloc(dd->request, sizeof(int*) * dd->n);

    if(dd->m!=0){
        dd->allocation[dd->n-1] = calloc(1, sizeof(int) * dd->m);
        dd->request[dd->n-1] = calloc(1, sizeof(int) * dd->m);
    }
    else{
        dd->allocation[dd->n-1] = NULL;
        dd->request[dd->n-1] = NULL;
    }
}

void DDProcTerm(t_deadlockDetector* dd, t_packet* newInfo){
    t_process* proc = (t_process*)streamTake_UINT32(newInfo->payload);

    dd->n--;
    int i = 0;
    while (dd->procs[i] != proc) i++;

    memmove(dd->procs+i, dd->procs+i+1, sizeof(t_process*) * (dd->n - i));
    dd->procs = realloc(dd->procs, sizeof(t_process*) * dd->n);

    free(dd->allocation[i]);
    memmove(dd->allocation+i, dd->allocation+i+1, sizeof(int*) * (dd->n - i));
    dd->allocation = realloc(dd->allocation, sizeof(int*) * dd->n);

    free(dd->request[i]);
    memmove(dd->request+i, dd->request+i+1, sizeof(int*) * (dd->n - i));
    dd->request = realloc(dd->request, sizeof(int*) * dd->n);
}

void(*deadlockHandlers[DD_MAX])(t_deadlockDetector* dd, t_packet* newInfo) =
{
    DDSemInit,
    DDSemAllocatedInstant,
    DDSemAllocated,
    DDSemRequested,
    DDSemRelease,
    DDSemDestroy,
    DDProcInit,
    DDProcTerm
};

bool findDeadlocks(t_deadlockDetector* dd, int memorySocket){
    bool isDeadlock = false;

    int *work = malloc(sizeof(int) * dd->m);
    memcpy(work, dd->available, dd->m * sizeof(int));
    bool *finish = malloc(sizeof(int) * dd->n);
    for(int i = 0; i < dd->n; i++){
        finish[i] = true;
        for(int j = 0; j < dd->m; j++){
            if(dd->allocation[i][j]){
                finish[i] = false;
                break;
            }
        }
    }
    for(int i = 0; i < dd->n; i++){
        bool found = true;
        for(int j = 0; j < dd->m; j++){
            if(dd->request[i][j] > work[j]){
                found = false;
                break;
            }
        }
        if(!finish[i] && found){
            for(int j = 0; j < dd->m; j++){
                work[j] += dd->allocation[i][j];
                finish[i] = true;
            }
        }
    }

    t_process* locked = NULL;
    for(int i = 0; i < dd->n; i++){
        if(!finish[i]){
            isDeadlock = true;
            if(locked == NULL) locked = dd->procs[i];
            if(locked->pid <= dd->procs[i]->pid) locked = dd->procs[i];
        }
    }
    
    bool matchesPid(void* elem){
        return locked->pid == ((t_process*)elem)->pid;
    };
    
    free(work);
    free(finish);
    
    if (!isDeadlock){
        return false;
    }

    for(int i = 0; i < dd->n; i++){
        if(locked == dd->procs[i]){
            for(int j = 0; j < dd->m; j++){
                if(dd->request[i][j]){
                    pthread_mutex_lock(&mutex_mediumTerm);
                    pQueue_removeBy(dd->sems[j]->waitingProcesses, matchesPid);
                    if(locked->state == BLOCKED) pQueue_removeBy(blockedQueue, matchesPid);
                    else if(locked->state == SUSP_BLOCKED) pQueue_removeBy(suspendedBlockedQueue, matchesPid);
                    pthread_mutex_unlock(&mutex_mediumTerm);
                    dd->request[i][j] = 0;
                }
                while(dd->allocation[i][j]){
                    mateSem_post(dd->sems[j]);
                    dd->allocation[i][j]--;
                    dd->available[j]++;
                }
            }

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "DEADLOCK: proceso %i terminado", dd->procs[i]->pid);
            pthread_mutex_unlock(&mutex_log);

            t_packet* termMemory = createPacket(CAPI_TERM, INITIAL_STREAM_SIZE);
            streamAdd_UINT32(termMemory, dd->procs[i]->pid);
            socket_sendPacket(memorySocket, termMemory);
            destroyPacket(termMemory);

            t_packet* response = createPacket(ERROR, 0);
            socket_sendPacket(dd->procs[i]->socket, response);
            destroyPacket(response);

            destroyProcess(dd->procs[i]);
            sem_post(&sem_multiprogram);

            dd->n--;
            memmove(dd->procs+i, dd->procs+i+1, sizeof(t_process*) * (dd->n - i));
            dd->procs = realloc(dd->procs, sizeof(t_process*) * dd->n);

            free(dd->allocation[i]);
            memmove(dd->allocation+i, dd->allocation+i+1, sizeof(int*) * (dd->n - i));
            dd->allocation = realloc(dd->allocation, sizeof(int*) * dd->n);
        
            free(dd->request[i]);
            memmove(dd->request+i, dd->request+i+1, sizeof(int*) * (dd->n - i));
            dd->request = realloc(dd->request, sizeof(int*) * dd->n);

            break;
        }
    }
    return true;
}