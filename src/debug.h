#ifndef MON_C_INTERPRETER_DEBUG_H
#define MON_C_INTERPRETER_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif //MON_C_INTERPRETER_DEBUG_H
