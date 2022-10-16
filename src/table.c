

#include "table.h"
#include "memory.h"

void initTable(Table* table) {
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}