#include <stdint.h>
#include <stdbool.h>

typedef struct pageTableEntry{
    bool present;
    uint32_t frame;
}t_pageTableEntry;

typedef struct pageTable{
    int pageQuantity;
    t_pageTableEntry *entries;
}t_pageTable;

t_pageTable *initializePageTable();

void destroyPageTable(t_pageTable *table);

int32_t pageTableAddEntry(t_pageTable *table, uint32_t newFrame);