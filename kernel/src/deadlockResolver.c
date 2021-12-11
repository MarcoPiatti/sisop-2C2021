#include "kernel.h"

t_list* seen = NULL;
bool isInCircularWait(t_deadlockDetector* dd, int i, int base){
    bool found = false;
    for(int j = 0; j < dd->m; j++){
        if(dd->request[i][j]){
            for(int next = 0; next < dd->n; next++){
                if(dd->allocation[next][j]){
                    if(next == base) return true;
                    bool matchesPid(void* elem){
                        return dd->procs[next]->pid == ((t_process*)elem)->pid;
                    };
                    if(list_find(seen, matchesPid) != NULL) return false;
                    list_add(seen, dd->procs[next]);
                    found = found || isInCircularWait(dd, next, base);
                }
            }
        }
    }
    return found;
}

/**
 * @DESC: La funcion mas importante del DD.
 * Segun sus matrices busca procesos que pidan mas recursos de los que hay.
 * Si se da el caso, implicando un deadlock, termina al de mayor pid, y notifica a la memoria de esto.
 * Si todo esto sucedio, retorna true para que externamente se vuelva a llamar a la funcion hasta que no haya deadlocks
 */
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
    for(int ii = 0; ii < dd->n; ii++){
        for(int i = 0; i < dd->n; i++){
            bool pideMasQueLoDisponible = false;
            for(int j = 0; j < dd->m; j++){
                if(dd->request[i][j] > work[j]){
                    pideMasQueLoDisponible = true;
                    break;
                }
            }
            if(!finish[i] && !pideMasQueLoDisponible){
                for(int j = 0; j < dd->m; j++){
                    work[j] += dd->allocation[i][j];
                    finish[i] = true;
                }
            }
        }
    }
    
    void* biggestPid(void* elem1, void* elem2){
        if (((t_process*)elem1)->pid > ((t_process*)elem2)->pid){
            return elem1;
        }
        else return elem2;
    };
    
    t_list* inCircularWait = list_create();
    seen = list_create();
    for(int i = 0; i < dd->n; i++){
        if(!finish[i]){
            isDeadlock = true;
            if(isInCircularWait(dd, i, i)){
                list_add(inCircularWait, dd->procs[i]);
            }
            list_clean(seen);
        }
    }

    if (!isDeadlock){
        free(work);
        free(finish);
        list_destroy(seen);
        return false;
    }

    t_process* locked = list_get_maximum(inCircularWait, biggestPid);
    list_destroy(seen);

    free(work);
    free(finish);

    bool matchesPid(void* elem){
        return locked->pid == ((t_process*)elem)->pid;
    };

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
            streamAdd_UINT32(termMemory->payload, dd->procs[i]->pid);
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