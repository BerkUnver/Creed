#ifndef CREED_TOKEN_H
#define CREED_TOKEN_H

#include <stdio.h>

#define ID_MAX_LENGTH 64
#define OPERATOR_MAX_LENGTH 2

#define STR_EQUALS "=="
#define STR_ASSIGN "="

#define STR_IF "if"
#define STR_ELIF "elif"
#define STR_ELSE "else"

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
    // There must be a token with the value 256 so everything after it has a higher value.

    TOKEN_EQUALS,
    TOKEN_ASSIGN,
    
    TOKEN_POINTER_REFERENCE,
    TOKEN_POINTER_DEREFERENCE,
   
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,

    TOKEN_ID,

    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_FLOAT,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
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
