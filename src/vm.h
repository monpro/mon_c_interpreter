#ifndef MON_C_INTERPRETER_VM_H
#define MON_C_INTERPRETER_VM_H

#include "chunk.h"

#define STACK_MAX 256
typedef struct {
    Chunk* chunk;
    // instruction pointer
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;

} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);
void push(Value value);
Value pop();
#endif //MON_C_INTERPRETER_VM_H
