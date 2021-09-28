#ifndef PAGETABLE_H_
#define PAGETABLE_H_

#include <commons/collections/dictionary.h>
#include <stdint.h>

typedef struct pageTable{
    t_dictionary* tables;
} t_pageTable;

t_pageTable* pageTable_create();

void pageTable_destroy(t_pageTable* self);

void pageTable_addProcess(t_pageTable* self, uint32_t pid);

void pageTable_removeProcess(t_pageTable* self, uint32_t pid);

void pageTable_addPage(t_pageTable* self, uint32_t pid, int32_t frame, bool isPresent);

void pageTable_removePage(t_pageTable* self, uint32_t pid, int32_t page);

int pageTable_countPages(t_pageTable* self, uint32_t pid);

bool pageTable_isPresent(t_pageTable* self, uint32_t pid, int32_t page);

void pageTable_setPresent(t_pageTable* self, uint32_t pid, int32_t page, bool isPresent);

int32_t pageTable_getFrame(t_pageTable* self, uint32_t pid, int32_t page);

void pageTable_setFrame(t_pageTable* self, uint32_t pid, int32_t page, int32_t frame);

#endif // !PAGETABLE_H_
