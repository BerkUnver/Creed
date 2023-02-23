#ifndef CREED_TOKEN_H
#define CREED_TOKEN_H

#include <stdio.h>

#define ID_MAX_LENGTH 64
#define OPERATOR_MAX_LENGTH 2

#define STR_EQUALS "=="
#define STR_ASSIGN "="

typedef enum TokenType {
    TOKEN_EOF = EOF,

    TOKEN_PAREN_OPEN = '(',
    TOKEN_PAREN_CLOSE = ')',
    TOKEN_CURLY_BRACE_OPEN = '{',
    TOKEN_CURLY_BRACE_CLOSE = '}',
    TOKEN_BRACKET_OPEN = '[',
    TOKEN_BRACKET_CLOSE = ']',
    
    TOKEN_COLON = ':',
    TOKEN_COMMA = ',',
    TOKEN_SEMICOLON = ';',
    TOKEN_DOT = '.',

    TOKEN_ERROR = 256,

    TOKEN_EQUALS = 257,
    TOKEN_ASSIGN = 258,
    
    TOKEN_IF = 259,
    TOKEN_ELSE = 260,
    TOKEN_POINTER_REFERENCE = 261,
    TOKEN_POINTER_DEREFERENCE = 262,
    TOKEN_ID = 263,

    TOKEN_LITERAL_STRING = 264,
    TOKEN_LITERAL_INT = 265,
    TOKEN_LITERAL_FLOAT = 266,

    TOKEN_PLUS = 267,
    TOKEN_MINUS = 268,
    TOKEN_MULTIPLY = 269,
    TOKEN_DIVIDE = 270,
    TOKEN_MODULO = 271,
} TokenType;

typedef union TokenData {
    int literal_integer;
    float literal_float;
    char *literal_string;
    const char* error; // FOR NOW, ERRORS ARE STATIC STRINGS. DO NOT FREE!
    char *id;
} TokenData;

typedef struct Token {
    TokenType type;
    TokenData data;
    int line_idx;
    int char_idx;
    int len;
} Token;

#define SINGLE_CHAR_TOKENS_COUNT 11
extern const TokenType single_char_tokens[SINGLE_CHAR_TOKENS_COUNT]; // needs to be int type because contains EOF

#define OPERATOR_CHARS_COUNT 11
extern const char operator_chars[OPERATOR_CHARS_COUNT];

#define WHITESPACE_CHARS_COUNT 4
extern const char whitespace_chars[WHITESPACE_CHARS_COUNT];

void token_print(Token *token);
void token_free(Token *token);

#endif
