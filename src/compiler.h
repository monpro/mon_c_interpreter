#ifndef MON_C_INTERPRETER_COMPILER_H
#define MON_C_INTERPRETER_COMPILER_H

#include "object.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();
#endif //MON_C_INTERPRETER_COMPILER_H
