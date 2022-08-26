#ifndef MON_C_INTERPRETER_VM_H
#define MON_C_INTERPRETER_VM_H

#include "chunk.h"

typedef struct {
    Chunk* chunk;
} VM;

void initVM();
void freeVM();

#endif //MON_C_INTERPRETER_VM_H
