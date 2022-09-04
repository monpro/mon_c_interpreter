#include <printf.h>
#include <stdlib.h>
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
Chunk* compilingChunk;

Chunk* currentChunk() {
    return compilingChunk;
}

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

void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void emitReturn() {
    emitByte(OP_RETURN);
}

void endCompiler() {
    emitReturn();
}


void expression() {

}

uint8_t makeConstant(double value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

void unary() {
    TokenType  operatorType = parser.previous.type;

    expression();

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return;
    }
}

void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

bool compile(const char* source, Chunk* chunk) {
    parser.panicMode = false;
    parser.hadError = false;
    initScanner(source);
    compilingChunk = chunk;
    advance();
    //expression();
    consume(TOKEN_EOF, "Expect the end of expression");
    endCompiler();
    return !parser.hadError;
}
