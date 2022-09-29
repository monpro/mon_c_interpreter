#include <printf.h>
#include <stdlib.h>
#include "scanner.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"

typedef struct {
    Token current;
    Token previous;
    // whether error happened during compilation
    bool hadError;
    // after first met error, we enter into panicMode
    bool panicMode;
} Parser;

typedef enum {
    // precedence level from low to high
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISION,// < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    // the function to compile an infix expression
    // whose left operand is followed by a token of that type.
    ParseFn infix;
    // the precedence of an infix expression that uses the token as an operator
    Precedence precedence;
} ParseRule;

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

static void advance() {
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
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

ParseRule* getRule(TokenType type);

uint8_t makeConstant(Value value) {
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

// when parse the precedence,
// only parsed those who have higher precedence
// eg: -a.b + c, parsePrecedence(PREC_UNARY) will stop at -a.b
// because PREC_TERM have lower precedence than PREC_UNARY
void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

void binary() {
    TokenType operatorType = parser.previous.type;

    ParseRule* rule = getRule(operatorType);

    parsePrecedence((Precedence)(rule -> precedence + 1));
    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return;
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(TOKEN_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(TOKEN_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(TOKEN_TRUE);
            break;
        default:
            return;
    }
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string() {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

void unary() {
    TokenType  operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        default:
            return;
    }
}

void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS] = {unary, binary, PREC_TERM},
        [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG] = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISION},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISION},
        [TOKEN_LESS] = {NULL, binary, PREC_COMPARISION},
        [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISION},
        [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
        [TOKEN_STRING] = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
        [TOKEN_AND] = {NULL, NULL, PREC_NONE},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, NULL, PREC_NONE},
        [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
        [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

ParseRule* getRule(TokenType type) {
    return &rules[type];
}


bool compile(const char* source, Chunk* chunk) {
    parser.panicMode = false;
    parser.hadError = false;
    initScanner(source);
    compilingChunk = chunk;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect the end of expression");
    endCompiler();
    return !parser.hadError;
}
