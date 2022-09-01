#include <printf.h>
#include "scanner.h"
#include "chunk.h"

// TODO: add advance, expression and consume
void compile(const char* source, Chunk* chunk) {
    initScanner(source);
    //advance();
    //expression();
    //consume(TOKEN_EOF, "Expect the end of expression");
    return;
}
