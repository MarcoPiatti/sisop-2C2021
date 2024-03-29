#include "tlb.h"
#include "memoryConfig.h"
#include "memory.h"
#include <unistd.h>

t_tlb* createTLB() {

    //Inicializo estructura de TLB
    tlb = malloc(sizeof(t_tlb));
    tlb->size = config->TLBEntryAmount;
    tlb->entries = (t_tlbEntry*) calloc(tlb->size, sizeof(t_tlbEntry));
    tlb->victimQueue = list_create();
    pthread_mutex_init(&tlb->mutex, NULL);
    tlb->pidHits = dictionary_create();
    tlb->pidMisses = dictionary_create();

    if(strcmp(config->TLBReplacementAlgorithm, "LRU") == 0) {
        tlb->updateVictimQueue = lruAlgorithm;
    } else {
        tlb->updateVictimQueue = fifoAlgorithm;
    }

    //Seteo todas las entradas como libres
    for(int i = 0; i < tlb->size; i++) {
        tlb->entries[i].isFree = true;
    }

    return tlb;
}

//Obtener frame segun pid y nPagina //TODO: Ver tema que es int32_t y no uint32_t (para posibilitar retorno -1)
int32_t getFrameFromTLB(uint32_t pid, uint32_t page) {
    pthread_mutex_lock(&tlb->mutex);
    int32_t frame = -1;
    for(int i = 0; i < tlb->size; i++) {
        if(tlb->entries[i].pid == pid && tlb->entries[i].page == page && tlb->entries[i].isFree == false) {
            tlb->updateVictimQueue(&(tlb->entries[i]));
            frame = tlb->entries[i].frame;
        }
    }

    //Agrego a metricas
    char* key = string_itoa(pid);
    if(frame == -1) {
        addToMetrics(tlb->pidMisses, key);

        pthread_mutex_lock(&logMut);
        log_info(logger, "TLB Miss: Proceso %s, numero de pagina %u", key, page);
        pthread_mutex_unlock(&logMut);
    } else {
        addToMetrics(tlb->pidHits, key);
        pthread_mutex_lock(&logMut);
        log_info(logger, "TLB Hit: Proceso %s, numero de pagina %u, marco %u", key, page, frame);
        pthread_mutex_unlock(&logMut);
    }

    pthread_mutex_unlock(&tlb->mutex);
    free(key);
    return frame;
}

void addEntryToTLB(uint32_t pid, uint32_t page, int32_t frame) {
    pthread_mutex_lock(&tlb->mutex);

    //Busco si hay una entrada libre
    _Bool freeFound = false;
    for(int i = 0; i < tlb->size && !freeFound; i++) {
        if(tlb->entries[i].isFree) {
            tlb->entries[i].pid = pid;
            tlb->entries[i].page = page;
            tlb->entries[i].frame = frame;
            tlb->entries[i].isFree = false;

            pthread_mutex_lock(&logMut);
            log_debug(logger, "TLB: Entrada libre ocupada %d PID: %u, Pagina: %u, Marco %u", i, pid, page, frame);
            pthread_mutex_unlock(&logMut);

            freeFound = true;
            list_add(tlb->victimQueue, &(tlb->entries[i]));
            break;
        }
    }

    //Si no hay entrada libre, reemplazo
    if(!freeFound) {
        if(!list_is_empty(tlb->victimQueue)){
            t_tlbEntry* victim = list_remove(tlb->victimQueue, 0);

            pthread_mutex_lock(&logMut);
            log_info(logger, "TLB, reemplazo de entrada %d. Sale PID: %u, numero de pagina: %u, marco: %u // Entra PID: %u, numero de pagina: %u, marco: %u", 
            (victim - tlb->entries) ,victim->pid, victim->page, victim->frame, pid, page, frame);
            pthread_mutex_unlock(&logMut);

            victim->pid = pid;
            victim->page = page;
            victim->frame = frame;
            victim->isFree = false;
            list_add(tlb->victimQueue, victim);
        }
    }

    pthread_mutex_unlock(&tlb->mutex);
}


void lruAlgorithm(t_tlbEntry* entry) {

    bool isVictim(t_tlbEntry* victim) {
        return victim == entry;
    }

    t_tlbEntry* entryToBeMoved = list_remove_by_condition(tlb->victimQueue, (void*) isVictim);

    list_add(tlb->victimQueue, entryToBeMoved);

}

void fifoAlgorithm(t_tlbEntry* entry) {
    //Intencionalmente vacío
}

//No libera la entrada, si no que la marca como libre
void dropEntry(uint32_t pid, uint32_t page) {
    pthread_mutex_lock(&tlb->mutex);
    for(int i = 0; i < tlb->size; i++) {
        if(tlb->entries[i].pid == pid && tlb->entries[i].page == page) {
            tlb->entries[i].isFree = true;
            pthread_mutex_unlock(&tlb->mutex);
            return;
        }
    }
    pthread_mutex_unlock(&tlb->mutex);
}

void destroyTLB(t_tlb* tlb) {
    //TODO: Chequear pls
    free(tlb->entries);
    list_destroy(tlb->victimQueue);
    pthread_mutex_destroy(&tlb->mutex);

    dictionary_destroy_and_destroy_elements(tlb->pidHits, free);
    dictionary_destroy_and_destroy_elements(tlb->pidMisses, free);
    free(tlb);
}

void cleanTLB() {
    pthread_mutex_lock(&tlb->mutex);
    for(int i = 0; i < tlb->size; i++) {
        tlb->entries[i].isFree = true;
    }
    pthread_mutex_unlock(&tlb->mutex);
}

//Libera las entradas de un proceso específico
void freeProcessEntries(uint32_t pid) {
    pthread_mutex_lock(&tlb->mutex);
    for(int i = 0; i < tlb->size; i++) {
        if(tlb->entries[i].pid == pid) {
            tlb->entries[i].isFree = true;
        }
    }

    pthread_mutex_lock(&logMut);
    log_info(logger, "TLB: Liberadas entradas del proceso %u", pid);
    pthread_mutex_unlock(&logMut);

    pthread_mutex_unlock(&tlb->mutex);
}


// --------------- Metricas -----------------


void addToMetrics(t_dictionary* dic, char* pid) {
    if(dictionary_has_key(dic, pid)) {
        uint32_t* value = (uint32_t*) dictionary_get(dic, pid);
        *value = *value + 1;
    }
    else {
        uint32_t* value = malloc(sizeof(uint32_t));
        *value = 1;
        dictionary_put(dic, pid, value);
    }
}


//Esta funcion SOLO IMPRIME las metricas pedidas del sigint
void sigIntHandlerTLB(int unused) {
    pthread_mutex_lock(&tlb->mutex);
    printf("-----------------------------------------\n");
    
    uint32_t hits = sum_dictionary_values(tlb->pidHits);

    printf("Cantidad total de hits: %d\n", hits);
    printTlbHits(tlb->pidHits);

    uint32_t misses = sum_dictionary_values(tlb->pidMisses);

    printf("Cantidad total de misses: %d\n", misses);
    printTlbMisses(tlb->pidMisses);
    

    printf("-----------------------------------------\n");
    pthread_mutex_unlock(&tlb->mutex);
    close(serverSocket);
    exit(EXIT_SUCCESS);
}

//Repeticion de logica? :P
void printTlbHits(t_dictionary* hits) {
    void print(char* key, void* hits) {
        printf("PID %s: %u hits\n", key, *(uint32_t*)hits);
    }
    dictionary_iterator(hits, print);
}

void printTlbMisses(t_dictionary* misses) {
    void print(char* key, void* misses) {
        printf("PID %s: %u misses\n", key, *(uint32_t*)misses);
    }
    dictionary_iterator(misses, print);
}

// --------------- SIGUSRs -----------------

void sigUsr1HandlerTLB(int unused) {
    char* dirPath = config->TLBPathDump;
    char* filePath = (char*) calloc(strlen(dirPath) + 35, sizeof(char));

    //Get time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char* timestamp = temporal_get_string_time("%H_%M_%S_%MS");

    sprintf(filePath, "%s/Dump_%s.tlb", dirPath, timestamp);

    FILE* f = fopen(filePath, "w");
    if(f == NULL) {
        pthread_mutex_lock(&logMut);
        log_error(logger, "Error al abrir el archivo de dump de TLB %s", filePath);
        pthread_mutex_unlock(&logMut);
        return;
    } else {
        pthread_mutex_lock(&logMut);
        log_debug(logger, "TLB: Generado dump en %s", filePath);
        pthread_mutex_unlock(&logMut);
    }
    
    pthread_mutex_lock(&tlb->mutex);

    fprintf(f, "%d/%d/%d %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    for(int i = 0; i<tlb->size; i++) {
        printTLBEntry(f, &tlb->entries[i], i);
    }

    pthread_mutex_unlock(&tlb->mutex);
    
    fclose(f);
    free(filePath);
    free(timestamp);
    //TODO
}

void printTLBEntry(FILE* f, t_tlbEntry* entry, int nEntry) {
    char status[10];
    if(entry->isFree) strcpy(status,"libre"); else strcpy(status, "ocupado");
    fprintf(f, "Entrada: %d\t Estado: %s\t Carpincho: %u\t Pagina: %u\t Marco: %d\n", nEntry, status, entry->pid, entry->page, entry->frame);
}

void sigUsr2HandlerTLB(int unused) {
    pthread_mutex_lock(&logMut);
        log_info(logger, "Se limpio la TLB.");
    pthread_mutex_unlock(&logMut);

    cleanTLB();
}

//Aux

uint32_t sum_dictionary_values(t_dictionary* dict) {
    uint32_t sum = 0;
    void sum_values(char* key, void* value) {
        sum += *(uint32_t*)value;
    }
    dictionary_iterator(dict, sum_values);
    return sum;
}