#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
void initChunk(Chunk* chunk) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
    initValueArray(&chunk->constants);
}

void writeChunk(Chunk* chunk, uint8_t byte) {
 if (chunk->capacity < chunk->count + 1) {
     int capacity = chunk->capacity;
     chunk->capacity = GROW_CAPACITY(capacity);
     chunk->code = GROW_ARRAY(uint8_t, chunk->code, capacity, chunk->capacity);
 }
 chunk->code[chunk->count] = byte;
 chunk->count++;
}

void freeChunk(Chunk* chunk)  {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}