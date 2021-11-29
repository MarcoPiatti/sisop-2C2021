#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>

pthread_mutex_t pageTablesMut;
t_dictionary* pageTables;

typedef struct pageTableEntry {
    bool present;
    uint32_t frame;
} t_pageTableEntry;

typedef struct pageTable {
    int pageQuantity;
    t_pageTableEntry *entries;
} t_pageTable;

t_pageTable *initializePageTable();

void destroyPageTable(t_pageTable *table);
void _destroyPageTable(void *table);

int32_t pageTableAddEntry(t_pageTable *table, uint32_t newFrame);

t_pageTable* getPageTable(uint32_t _PID, t_dictionary* pageTables);

bool pageTable_isEmpty(uint32_t PID);
