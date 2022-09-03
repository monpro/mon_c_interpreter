#include <printf.h>
#include "scanner.h"
#include "chunk.h"

typedef struct {
    Token current;
    Token previous;
} Parser;

Parser parser;

void errorAt(Token *token, const char *msg) {
    //TODO
}

void error(const char *msg) {
    errorAt(&parser.current, msg);
}

void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        error(parser.current.start);
    }
}

// TODO: add advance, expression and consume
void compile(const char* source, Chunk* chunk) {
    initScanner(source);
    //advance();
    //expression();
    //consume(TOKEN_EOF, "Expect the end of expression");
    return;
}
