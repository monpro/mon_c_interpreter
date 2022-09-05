#include <stdbool.h>
#include <libc.h>
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

// char*
void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

bool isAtEnd() {
    return *scanner.current == '\0';
}

Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)(strlen(message));
    token.line = scanner.line;
    return token;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

char peek() {
    return *scanner.current;
}

char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

void skipWhiteSpace() {
    for(;;) {
        char ch = peek();
        switch (ch) {
            case ' ' :
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated String.");
    advance();
    return makeToken(TOKEN_STRING);
}


bool isDigit(char ch) {
    return ch >= '0' && ch <= '9';
}

static Token number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

bool isAlpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

TokenType checkKeyWord(int start, int length, const char *rest, TokenType tokenType) {
    if (scanner.current - scanner.start == start + length &&
            memcmp(scanner.start + start, rest, length) == 0) {
        return tokenType;
    }
    return TOKEN_IDENTIFIER;
}

TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyWord(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyWord(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyWord(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyWord(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyWord(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyWord(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyWord(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyWord(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyWord(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyWord(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyWord(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyWord(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyWord(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyWord(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyWord(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyWord(1, 4, "hile", TOKEN_WHILE);

    }
}

Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

Token scanToken() {
    skipWhiteSpace();
    scanner.start = scanner.current;
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char ch = advance();
    if (isAlpha(ch)) return identifier();
    if (isDigit(ch)) return number();
    switch (ch) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '+': return makeToken(TOKEN_PLUS);
        case '-': return makeToken(TOKEN_MINUS);
        case '*': return makeToken(TOKEN_STAR);
        case '/': return makeToken(TOKEN_SLASH);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return string();
    }
    //TODO: add token scan logic
    return errorToken("Unexpected character.");
}