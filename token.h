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
    TOKEN_DOT = '.', // when this appears in a literal float, we consume it as part of the float.
    // To prevent the ambiguity between the '.' at the beginning of a decimal and the '.' used to access data structure members:
    // We just don't allow having a decimal that starts only with '.', must start with "0."

    // There must be a token with the value 256 so everything after it has a higher value.
    TOKEN_EQUALS = 256,
    TOKEN_ASSIGN,
    
    TOKEN_POINTER_REFERENCE,
    TOKEN_POINTER_DEREFERENCE,
   
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,

    TOKEN_ID,

    TOKEN_LITERAL_CHAR,
    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_DOUBLE,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
} TokenType;

typedef union TokenData {
    int literal_int; // add types for int8, int16, int64
    double literal_double; // add types for float, float64
    char *literal_string;
    char literal_char;
    char *id;
} TokenData;

typedef struct Token {
    TokenType type;
    TokenData data;
    int line_idx;
    int char_idx;
    int len;
} Token;

// c is an int so you can also pass a TokenType itself in here instead of a char.
bool char_is_single_char_token_type(int c);

bool char_is_whitespace(char c);
bool char_is_operator(char c);
bool char_is_identifier(char c);

void token_print(Token *token);
void token_free(Token *token);

#endif
