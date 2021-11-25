#include "pageTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>

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
void _destroyPageTable(void *table){
    destroyPageTable((t_pageTable*) table);
}


int32_t pageTableAddEntry(t_pageTable *table, uint32_t newFrame){
    pthread_mutex_lock(&pageTablesMut);
        table->entries = realloc(table->entries, sizeof(t_pageTableEntry)*(table->pageQuantity + 1));
        (table->entries)[table->pageQuantity].frame = newFrame;
        (table->entries)[table->pageQuantity].present = false;
        (table->pageQuantity)++;
        int32_t pgQty = table->pageQuantity -1;
    pthread_mutex_unlock(&pageTablesMut);
    return pgQty;
}

t_pageTable* getPageTable(uint32_t _PID, t_dictionary* pageTables) {
    char *PID = string_itoa(_PID);

    t_pageTable* pt = (t_pageTable*) dictionary_get(pageTables, PID);

    return pt;
}