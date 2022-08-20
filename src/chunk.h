#ifndef MON_C_INTERPRETER_CHUNK_H
#define MON_C_INTERPRETER_CHUNK_H

#include "common.h"
typedef enum {
    OP_RETURN
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
} Chunk;

#endif //MON_C_INTERPRETER_CHUNK_H
