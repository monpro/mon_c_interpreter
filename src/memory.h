
#ifndef MON_C_INTERPRETER_MEMORY_H
#define MON_C_INTERPRETER_MEMORY_H

#include "common.h"
#include "value.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) \
 ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
 (type*)reallocate(pointer, sizeof(type) * (oldCount),\
 sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void* reallocate(void * pinter, size_t oldSize, size_t newSize);
void collectGarbage();
void freeObjects();
void markValue(Value value);
void markObject(Obj* obj);
#endif //MON_C_INTERPRETER_MEMORY_H
