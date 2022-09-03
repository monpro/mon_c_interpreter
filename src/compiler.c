#include <printf.h>
#include "scanner.h"
#include "chunk.h"

typedef struct {
    Token current;
    Token previous;
    // whether error happened during compilation
    bool hadError;
    // after first met error, we enter into panicMode
    bool panicMode;
} Parser;

Parser parser;

void errorAt(Token *token, const char *msg) {
    if (parser.panicMode == true) {
        return;
    }
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, "at end");
    } else if (token->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, "at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", msg);
    parser.hadError = true;
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

// TODO: add expression and consume
void consume(TokenType type, const char *msg) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    error(msg);
}

bool compile(const char* source, Chunk* chunk) {
    parser.panicMode = false;
    parser.hadError = false;
    initScanner(source);
    advance();
    //expression();
    consume(TOKEN_EOF, "Expect the end of expression");
    return !parser.hadError;
}
