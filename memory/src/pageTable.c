#include "pageTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>

t_pageTable *initializePageTable(){
    t_pageTable *newTable = malloc(sizeof(t_pageTable));
    newTable->pageQuantity = 0;
    newTable->entries = NULL;
    return newTable;
}

void destroyPageTable(t_pageTable *table){
    free(table->entries);
    free(table);
}

int32_t pageTableAddEntry(t_pageTable *table, uint32_t newFrame){
    table->entries = realloc(table->entries, sizeof(t_pageTableEntry)*(table->pageQuantity + 1));
    (table->entries)[table->pageQuantity]->frame = newFrame;
    (table->entries)[table->pageQuantity]->present = false;
    (table->pageQuantity)++;
    return table->pageQuantity -1;
}

t_pageTable* getPageTable(uint32_t _PID, t_dictionary* pageTables) {
    char *PID = string_itoa(_PID);
    return (t_pageTable*) dictionary_get(pageTables, PID);
}