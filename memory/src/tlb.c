#include "tlb.h"
#include "memoryConfig.h"
#include "memory.h"

//TODO: Implementar sigUsrs 

t_tlb* createTLB() {

    //Inicializo estructura de TLB
    tlb = malloc(sizeof(t_tlbEntry));
    tlb->size = config->TLBEntryAmount;
    tlb->entries = (t_tlbEntry*) calloc(sizeof(t_tlbEntry), tlb->size);
    tlb->fifoEntries = queue_create();
    pthread_mutex_init(&tlb->mutex, NULL);
    tlb->pidHits = dictionary_create();
    tlb->pidMisses = dictionary_create();

    if(strcmp(config->TLBReplacementAlgorithm, "LRU") == 0) {
        tlb->replaceAlgorithm = lruAlgorithm;
    } else {
        tlb->replaceAlgorithm = fifoAlgorithm;
    }

    //Seteo todas las entradas como libres
    for(int i = 0; i < config->TLBEntryAmount; i++) {
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
            clock_gettime(CLOCK_MONOTONIC, tlb->entries[i].lastAccess);
            frame = tlb->entries[i].frame;
        }
    }

    //Agrego a metricas
    char key[10];
    sprintf(key, "&#37;u", pid);
    printf("Agregando key %u\n", key);//TODO: Sacar
    if(frame == -1) {
        addToMetrics(tlb->pidMisses, key);
    } else {
        addToMetrics(tlb->pidHits, key);
    }

    pthread_mutex_unlock(&tlb->mutex);
    return frame;
}

uint32_t addEntryToTLB(uint32_t pid, uint32_t page, int32_t frame) {
    //Genero la entrada
    t_tlbEntry* newEntry;
    newEntry = malloc(sizeof(t_tlbEntry));
    newEntry->pid = pid;
    newEntry->page = page;
    newEntry->frame = frame;

    //Reemplazo/Agrego segun algoritmo
    pthread_mutex_lock(&tlb->mutex);
    tlb->replaceAlgorithm(newEntry);
    pthread_mutex_unlock(&tlb->mutex);


    //Libero la entrada auxiliar
    free(newEntry);
}


void lruAlgorithm(t_tlbEntry* newEntry) {
    //Fetch de la entrada a la que hace mas tiempo no se accede
    t_tlbEntry* entryToBeReplaced = tlb->entries;
    _Bool cont = true;
    for(int i=0; i < tlb->size-1 && cont; i++) {

        //Ver si la entrada esta libre
        if(entryToBeReplaced->isFree) {
            cont = false;
        }
        //Ver si la entrada lleva más tiempo sin ser accedida
        else if(tlb->entries[i].lastAccess->tv_sec > tlb->entries[i+1].lastAccess->tv_sec) {
            entryToBeReplaced = &tlb->entries[i+1];
        }
    }

    //Reemplazo de la entrada
    entryToBeReplaced->pid = newEntry->pid;
    entryToBeReplaced->page = newEntry->page;
    entryToBeReplaced->frame = newEntry->frame;
    entryToBeReplaced->isFree = false;
    clock_gettime(CLOCK_MONOTONIC, entryToBeReplaced->lastAccess);

}

void fifoAlgorithm(t_tlbEntry* newEntry) {
    //Se controla si hay alguna entrada libre
    for(int i = 0; i < tlb->size; i++) {
        if(tlb->entries[i].isFree) {
            tlb->entries[i].pid = newEntry->pid;
            tlb->entries[i].page = newEntry->page;
            tlb->entries[i].frame = newEntry->frame;
            tlb->entries[i].isFree = false;
            clock_gettime(CLOCK_MONOTONIC, tlb->entries[i].lastAccess); //No necesario
            return;
        }
    }


    //Fetch de la entrada que hace mas tiempo se encuentra cargada
    t_tlbEntry* entryToBeReplaced = queue_pop(tlb->fifoEntries);

    //Reemplazo de la entrada
    entryToBeReplaced->pid = newEntry->pid;
    entryToBeReplaced->page = newEntry->page;
    entryToBeReplaced->frame = newEntry->frame;
    entryToBeReplaced->isFree = false;
    clock_gettime(CLOCK_MONOTONIC, entryToBeReplaced->lastAccess); //No necesario
    queue_push(tlb->fifoEntries, entryToBeReplaced);


}

//No libera la entrada, si no que la marca como libre
void dropEntry(uint32_t pid, uint32_t page) {
    pthread_mutex_lock(&tlb->mutex);
    for(int i = 0; i < tlb->size; i++) {
        if(tlb->entries[i].pid == pid && tlb->entries[i].page == page) {
            tlb->entries[i].isFree = true;
            return;
        }
    }
    pthread_mutex_unlock(&tlb->mutex);
}

void destroyTLB(t_tlb* tlb) {
    //TODO: Chequear pls
    free(tlb->entries);
    queue_destroy(tlb->fifoEntries);
    pthread_mutex_destroy(&tlb->mutex);

    void destroyUint32(void* data) {
        free((*uint32_t)data);
    }
    dictionary_destroy_and_destroy_elements(tlb->pidHits, destroyUint32);
    dictionary_destroy_and_destroy_elements(tlb->pidMisses, destroyUint32);
    free(tlb);
}

void cleanTLB() {
    pthread_mutex_lock(&tlb->mutex);
    for(int i = 0; i < tlb->size; i++) {
        tlb->entries[i].isFree = true;
    }
    pthread_mutex_unlock(&tlb->mutex);
}

// --------------- Metricas -----------------


void addToMetrics(t_dictionary* dic, uint32_t pid) {
    if(dictionary_has_key(dic, &pid)) {
        uint32_t* value = (uint32_t*) dictionary_get(dic, &pid);
        *value = *value + 1;
        dictionary_put(dic, &pid, value);
    }
    else {
        uint32_t* value = malloc(sizeof(uint32_t));
        *value = 1;
        dictionary_put(dic, &pid, value);
    }
}


//Esta funcion SOLO IMPRIME las metricas pedidas del sigint
void sigIntHandlerTLB() {
    pthread_mutex_lock(&tlb->mutex);
    printf("-----------------------------------------\n");
    
    printf("Cantidad total de hits: %d\n", list_size(tlb->pidHits));
    printTlbHits(tlb->pidHits);

    printf("Cantidad total de misses: %d\n", list_size(tlb->pidMisses));
    printTlbMisses(tlb->pidMisses);
    

    printf("-----------------------------------------\n");
    pthread_mutex_unlock(&tlb->mutex);
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

void sigUsr1HandlerTLB() {
    char* dirPath = config->TLBPathDump;
    char* filePath = (char*) calloc(strlen(dirPath) + 35, sizeof(char));

    //Get time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char* timestamp = (char*) calloc(35, sizeof(char));
    sprintf(timestamp, "%d_%d_%d__%d_%d_%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    sprintf(filePath, "%s/Dump_%s.tlb", dirPath, timestamp);

    FILE* f = fopen(config->TLBPathDump, "w");
    printf("-----------------------------------------\n");
    pthread_mutex_lock(&tlb->mutex);

    fprintf(f, "%d/%d/%d %d:%d:%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    for(int i = 0; i<tlb->size; i++) {
        printTLBEntry(f, &tlb->entries[i], i);
    }

    pthread_mutex_unlock(&tlb->mutex);
    printf("-----------------------------------------\n");

    free(filePath);
    free(timestamp);
    //TODO
}

void printTLBEntry(FILE* f, t_tlbEntry* entry, int nEntry) {
    char status[10] = entry->isFree ? "Libre" : "Ocupado";
    fprintf(f, "Entrada: %d\t Estado: %s\t Carpincho: %u\t Pagina: %u\t Marco: %d\n", nEntry, status, entry->pid, entry->page, entry->frame);
}

void sigUsr2HandlerTLB() {
    cleanTLB();
}