#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
void initChunk(Chunk* chunk) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
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

