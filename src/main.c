#include <stdlib.h>
#include "chunk.h"

int main() {
    Chunk* chunk;
    chunk = malloc(sizeof(Chunk));
    initChunk(chunk);
    return 0;
}
