#ifndef CREED_TOKEN_H
#define CREED_TOKEN_H

#include <stdio.h>
#include <stdbool.h>

#define ID_MAX_LENGTH 64
#define OPERATOR_MAX_LENGTH 2

#define STR_EQUALS "=="
#define STR_ASSIGN "="

#define STR_IF "if"
#define STR_ELIF "elif"
#define STR_ELSE "else"
#define DELIMITER_LITERAL_STRING '"'
#define DELIMITER_LITERAL_CHAR '\''

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

extern const TokenType single_char_tokens[11];
bool char_is_whitespace(char c);
bool char_is_escape(char c);
bool char_is_operator(char c);

void token_print(Token *token);
void token_free(Token *token);

#endif
