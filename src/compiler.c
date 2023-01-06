#include <printf.h>
#include <stdlib.h>
#include <string.h>
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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    // the function to compile an infix expression
    // whose left operand is followed by a token of that type.
    ParseFn infix;
    // the precedence of an infix expression that uses the token as an operator
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth
} Local;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

typedef struct {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;


static void expression();
static void statement();
static void declaration();
static void varDeclaration();
Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

Chunk* currentChunk() {
    return &current->function->chunk;
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

static bool check(TokenType type) {
    return parser.current.type == type;
}
static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}



void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void emitReturn() {
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static ObjFunction *endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(),
                         function->name != NULL ? function->name->chars : "<script>");
    }
#endif
    current = (Compiler *) current->enclosing;
    return function;
}

ParseRule* getRule(TokenType type);

static void block();

static void declaration();

static int resolveLocal(Compiler *compiler, Token *name);

static void markInitialized();

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

static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
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
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target");
    }
}

static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = current->scopeDepth;
    local->depth = -1;
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable() {
    // global variables are bound at runtime, we do not track here
    if (current->scopeDepth == 0) return;
    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("already have a variable with this name in the scope");
        }

    }
    addLocal(*name);
}

void binary(bool canAssign) {
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

static void literal(bool canAssign) {
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

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// Resolves the index of a local variable with the specified name.
// Returns the index of the local variable if found, or -1 if not found.
// The search starts from the last local variable and works backwards.
static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name))  {
            if (local->depth == -1) {
                error("Cannot read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}


static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}


static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static int emitJump(uint8_t instruction) {
    // Emit the given instruction byte to the current chunk.
    emitByte(instruction);
    // Emit two placeholder bytes to the current chunk. These will be used to store the jump offset later.
    emitByte(0xff);
    emitByte(0xff);
    // Return the index of the first placeholder byte in the current chunk.
    // This will be used to patch the jump offset later.
    return currentChunk()->count - 2;
}

static void patchJump(int offset) {
    // Calculate the jump offset as the distance from the jump instruction to the jump destination.
    int jump = currentChunk()->count - offset - 2;

    // Check if the jump offset is too large to fit in 16 bits.
    if (jump > UINT16_MAX) error("Too much code to jump over");

    // Update the placeholder bytes at the specified offset with the calculated jump offset.
    // The jump offset is stored as two bytes in big-endian order.
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;

}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    int elseJump = emitJump(OP_JUMP);
    emitByte(OP_POP);
    patchJump(thenJump);
    if (match(TOKEN_ELSE)) statement();
}


static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body to large.");
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    if (match(TOKEN_SEMICOLON)) {

    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OP_POP);

}

static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }
    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitJump(OP_POP);
    }
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }
    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    endScope();
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if(match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    }
    else {
        expressionStatement();
    }
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ;
        }
        advance();
    }
}



static uint8_t parseVariable(const char *errorMsg) {
    consume(TOKEN_IDENTIFIER, errorMsg);
    declareVariable();
    if (current->scopeDepth > 0) return 0;
    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}


static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitJump(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect Variable name");
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error("Can't have more than 255 parameters");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body");
    block();

    ObjFunction* function = endCompiler();
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expect Function name");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void declaration() {
    if (match(TOKEN_FUN)) {
        funDeclaration();
    }
    else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) {
        synchronize();
    }
}

void unary(bool canAssign) {
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

void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}
ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
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
        [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
        [TOKEN_STRING] = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
        [TOKEN_AND] = {NULL, and_, PREC_NONE},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, or_, PREC_NONE},
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


ObjFunction* compile(const char* source) {
    parser.panicMode = false;
    parser.hadError = false;
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);
    advance();
//    expression();
//    consume(TOKEN_EOF, "Expect the end of expression");
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}
