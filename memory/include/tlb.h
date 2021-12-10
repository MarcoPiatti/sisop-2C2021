#include<stdint.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>
#include<pthread.h>
#include<commons/temporal.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>


typedef struct tlbEntry {
    uint32_t pid;
    uint32_t page;
    int32_t frame;
    _Bool isFree;
} t_tlbEntry;

typedef void (*TLBAlgorithm)(t_tlbEntry*);
typedef struct tlb {
    //Funcionamiento tlb
    t_tlbEntry* entries;
    unsigned int size;
    TLBAlgorithm updateVictimQueue;
    t_list* victimQueue;
    pthread_mutex_t mutex;

    //Metricas
    t_dictionary* pidHits;
    t_dictionary* pidMisses;

} t_tlb;

t_tlb* tlb;

//Funciones
t_tlb* createTLB();
int32_t getFrameFromTLB(uint32_t pid, uint32_t page);
void addEntryToTLB(uint32_t pid, uint32_t page, int32_t frame);
void dropEntry(uint32_t pid, uint32_t page);
void destroyTLB(t_tlb* tlb);
void freeProcessEntries(uint32_t pid);

//Metricas
void addToMetrics(t_dictionary* dic, char* pid);
void sigIntHandlerTLB();
void printTlbHits(t_dictionary* hits);
void printTlbMisses(t_dictionary* misses);
void sigUsr1HandlerTLB();
void printTLBEntry(FILE* f, t_tlbEntry* entry, int nEntry);
void sigUsr2HandlerTLB();

//Algoritmos de reemplazo
void lruAlgorithm(t_tlbEntry* entry);
void fifoAlgorithm(t_tlbEntry* entry);