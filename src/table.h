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
bool tableDelete(Table* table, ObjString* key);
bool tableGet(Table* table, ObjString* key, Value* value);
void tableAddAll(Table* from, Table* to);
void tableRemoveUnreachedStrings(Table* table);
void markTable(Table* table);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
#endif //MON_C_INTERPRETER_TABLE_H
