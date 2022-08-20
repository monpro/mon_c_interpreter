
#ifndef MON_C_INTERPRETER_MEMORY_H
#define MON_C_INTERPRETER_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(capacity) \
 ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
 (type*)reallocate(pointer, sizeof(type) * (oldCount),\
 sizeof(type) * (newCount))

void* reallocate(void * pinter, size_t oldSize, size_t newSize);

#endif //MON_C_INTERPRETER_MEMORY_H
