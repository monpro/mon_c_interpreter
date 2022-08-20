#include <stdio.h>

#include "memory.h"
#include "value.h"

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->count = 0;
    array->capacity = 0;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int capacity = array->capacity;
        array->capacity = GROW_CAPACITY(capacity);
        array->values = GROW_ARRAY(Value , array->values, capacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(ValueArray, array, array->capacity);
    initValueArray(array);
}