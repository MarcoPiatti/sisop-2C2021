#include "pageTable.h"
#include <stdlib.h>
#include <commons/string.h>
#include <string.h>

typedef struct entry{
    int32_t frame;
    bool present;
} t_entry;

typedef struct table{
    t_entry *pages;
    int32_t size;
} t_table;

void* createTable(){
    t_table* table = malloc(sizeof(t_table));
    table->pages = NULL;
    table->size = 0;
    return (void*)table;
}

void destroyTable(void* elem){
    t_table* table = (t_table*)elem;
    free(table->pages);
    free(table);
}

t_pageTable* pageTable_create(){
    t_pageTable* self = malloc(sizeof(t_pageTable*));
    self->tables = dictionary_create();
    return self;
}

void pageTable_destroy(t_pageTable* self){
    dictionary_destroy_and_destroy_elements(self->tables, destroyTable);
    free(self);
}

void pageTable_addProcess(t_pageTable* self, uint32_t pid){
    char* pidString = string_from_format("%u", pid);
    dictionary_put(self->tables, pidString, createTable());
    free(pidString);
}

void pageTable_removeProcess(t_pageTable* self, uint32_t pid){
    char* pidString = string_from_format("%u", pid);
    dictionary_remove_and_destroy(self->tables, pidString, destroyTable);
    free(pidString);
}

void pageTable_addPage(t_pageTable* self, uint32_t pid, int32_t frame, bool isPresent){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    table->size++;
    table->pages = realloc(table->pages, table->size * sizeof(t_entry));
    table->pages[table->size-1].frame = frame;
    table->pages[table->size-1].present = isPresent;
    free(pidString);
}

void pageTable_removePage(t_pageTable* self, uint32_t pid, int32_t page){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    table->size--;
    memmove(table->pages + page, table->pages + page + 1, sizeof(t_entry) * (table->size - page));
    table->pages = realloc(table->pages, table->size * sizeof(t_entry));
    free(pidString);
}

int pageTable_countPages(t_pageTable* self, uint32_t pid){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    free(pidString);
    return table->size;
}

bool pageTable_isPresent(t_pageTable* self, uint32_t pid, int32_t page){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    free(pidString);
    return table->pages[page].present;
}

void pageTable_setPresent(t_pageTable* self, uint32_t pid, int32_t page, bool isPresent){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    free(pidString);
    table->pages[page].present = isPresent;
}

int32_t pageTable_getFrame(t_pageTable* self, uint32_t pid, int32_t page){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    free(pidString);
    return table->pages[page].frame;
}

void pageTable_setFrame(t_pageTable* self, uint32_t pid, int32_t page, int32_t frame){
    char* pidString = string_from_format("%u", pid);
    t_table* table = dictionary_get(self->tables, pidString);
    free(pidString);
    table->pages[page].frame = frame;
}