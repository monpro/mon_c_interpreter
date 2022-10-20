#ifndef MON_C_INTERPRETER_TABLE_H
#define MON_C_INTERPRETER_TABLE_H

#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, ObjString* key, Value value);

#endif //MON_C_INTERPRETER_TABLE_H
