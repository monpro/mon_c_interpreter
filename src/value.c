#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "object.h"

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL: return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            ObjString* aString = AS_STRING(a);
            ObjString* bString = AS_STRING(b);

            return aString->length == bString->length && memcmp(aString, bString, aString->length) == 0;
        }

        default: return false;
    }
}

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