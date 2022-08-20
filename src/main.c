#include "chunk.h"
#include "debug.h"
int main() {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);
    disassembleChunk(&chunk, "test_OP_RETURN");
    freeChunk(&chunk);
    return 0;
}
