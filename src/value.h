#ifndef MON_C_INTERPRETER_VALUE_H
#define MON_C_INTERPRETER_VALUE_H
#include "common.h"

typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

#endif //MON_C_INTERPRETER_VALUE_H
