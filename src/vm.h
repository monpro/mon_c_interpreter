#ifndef MON_C_INTERPRETER_VM_H
#define MON_C_INTERPRETER_VM_H

#include "chunk.h"
#include "table.h"

#define STACK_MAX 256
typedef struct {
    Chunk* chunk;
    // instruction pointer
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Table strings;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();
#endif //MON_C_INTERPRETER_VM_H
