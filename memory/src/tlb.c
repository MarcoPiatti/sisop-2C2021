#include "tlb.h"
#include <stdlib.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/temporal.h>

typedef struct pidCount {
    uint32_t pid;
    int hits;
    int misses;
} t_pidCount;


void fifo(t_TLB* self, t_TLBEntry* entry){
    //Literalmente vacio, la cola de victimas ya es fifo de por si
    //Esto queda para ser bonito, ordenado y polimorfico (?) :) 
}

void lru(t_TLB* self, t_TLBEntry* entry){
    bool samePtr(void* elem){
        return elem == entry;
    };
    void* recentlyUsed = list_remove_by_condition(self->victims->elements, samePtr);
    queue_push(self->victims, recentlyUsed);
}

t_TLB* createTLB(int size, void (*algorithm)(t_TLB* self, t_TLBEntry* entry)){
    t_TLB* self = malloc(sizeof(t_TLB));
    self->size = size;
    self->algorithm = algorithm;
    self->counts = list_create();
    self->victims = queue_create();
    self->table = malloc(sizeof(t_TLBEntry) * size);
    for(int i = 0; i < self->size; i++){
        self->table[i].used = false;
    }
    pthread_mutex_init(&self->mutex, NULL);
    return self;
}

void destroyTLB(t_TLB* self){
    pthread_mutex_destroy(&self->mutex);
    list_destroy_and_destroy_elements(self->counts, free);
    queue_destroy(self->victims);
    free(self->table);
    free(self);
}

void addHit(t_list* counts, uint32_t pid){
    bool _samePid(void* elem){
        return ((t_pidCount*)elem)->pid == pid;
    };
    t_pidCount* entry = list_find(counts, _samePid);
    if(entry == NULL){
        entry = malloc(sizeof(t_pidCount));
        entry->pid = pid;
        entry->hits = 0;
        entry->misses = 0; 
        list_add(counts, entry);
    }
    entry->hits++;
}

void addMiss(t_list* counts, uint32_t pid){
    bool _samePid(void* elem){
        return ((t_pidCount*)elem)->pid == pid;
    };
    t_pidCount* entry = list_find(counts, _samePid);
    if(entry == NULL){
        entry = malloc(sizeof(t_pidCount));
        entry->pid = pid;
        entry->hits = 0;
        entry->misses = 0; 
        list_add(counts, entry);
    }
    entry->misses++;
}

int32_t TLB_findFrame(t_TLB* self, uint32_t pid, int32_t page){
    pthread_mutex_lock(&self->mutex);
    int32_t foundFrame = -1;
    for(int i = 0; i < self->size; i++){
        if(self->table[i].used && self->table[i].pid == pid && self->table[i].page == page){
            self->algorithm(self, self->table + i);
            foundFrame = self->table[i].frame;
            addHit(self->counts, pid);
            break;
        }
    }
    if (foundFrame == -1) addMiss(self->counts, pid);
    pthread_mutex_unlock(&self->mutex);
    return foundFrame;
}

void TLB_addEntry(t_TLB* self, uint32_t pid, int32_t page, int32_t frame){
    pthread_mutex_lock(&self->mutex);
    if(queue_size(self->victims) < self->size)
        for(int i = 0; i < self->size; i++)
            if(!self->table[i].used){
                self->table[i].pid = pid;
                self->table[i].page = page;
                self->table[i].frame = frame;
                self->table[i].used = true;
                queue_push(self->victims, self->table + i);
            }
    else{
        t_TLBEntry* victim = queue_pop(self->victims);
        victim->frame = frame;
        victim->page = page;
        victim->pid = pid;
        queue_push(self->victims, victim);
    }
    pthread_mutex_unlock(&self->mutex);
}

void TLB_clear(t_TLB* self){
    pthread_mutex_lock(&self->mutex);
    for(int i = 0; i < self->size; i++){
        self->table[i].used = false;
    }
    queue_clean(self->victims);
    pthread_mutex_unlock(&self->mutex);
}

void TLB_dump(t_TLB* self, char* dumpDir){
    pthread_mutex_lock(&self->mutex);
    char* time = temporal_get_string_time("%d-%m-%y_%H:%M:%S");
    char* fileName = string_from_format("Dump_%s.tlb", time);
    FILE* dumpFile = txt_open_for_append(fileName);

    char* text = string_from_format("Dump: %s\n", time);
    txt_write_in_file(dumpFile, text);
    free(text);
    for(int i = 0; i < self->size; i++){
        if(!self->table[i].used)
            text = string_from_format("Entrada:%-10i Estado:Libre      Proceso:-          Pagina:-          Marco:-         \n", i);
        else
            text = string_from_format("Entrada:%-10i Estado:Ocupado    Proceso:%-10u Pagina:%-10i Marco:%-10i\n",
                                        i, self->table[i].pid, self->table[i].page, self->table[i].frame);
        txt_write_in_file(dumpFile, text);
        free(text);
    }
    free(fileName);
    free(time);
    txt_close_file(dumpFile);
    pthread_mutex_unlock(&self->mutex);
}

void writeTotalHits(t_list* counts){
    int result = 0;
    void _Hits(void* elem){
        result += ((t_pidCount*)elem)->counts;
    };
    list_iterate(counts, _Hits);
    printf("TLB - Hits totales:%i\n", result);
    fflush(stdout);
}

void writeTotalMisses(t_list* counts){
    int result = 0;
    void _Misses(void* elem){
        result += ((t_pidCount*)elem)->misses;
    };
    list_iterate(counts, _Misses);
    printf("TLB - Miss totales:%i\n", result);
    fflush(stdout);
}

void writeHitsAndMisses(t_list* counts){
    void* printBoth(void* elem){
        t_pidCount* entry = (t_pidCount*)elem;
        printf("TLB - Proceso %-10u - Hits:%-10i - Misses:%-10i\n", entry->pid, entry->hits, entry->misses);
        fflush(stdout);
    };
}

void TLB_printCounts(t_TLB* self){
    pthread_mutex_lock(&self->mutex);
    writeTotalHits(self->counts);
    writeTotalMisses(self->counts);
    writeHitsAndMisses(self->counts);
    pthread_mutex_unlock(&self->mutex);
}