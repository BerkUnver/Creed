#ifndef CREED_TOKEN_H
#define CREED_TOKEN_H

#include <stdio.h>
#include <stdbool.h>

#define ID_MAX_LENGTH 64
#define OPERATOR_MAX_LENGTH 2

#define STR_ASSIGN "="

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
    // TOKEN_OP_MIN and TOKEN_OP_MAX aren't real tokens. They define the range of valid operators so we can do goofy stuff.
    // Example: puts(string_operators[token_type - TOKEN_OP_MIN]);
    TOKEN_OP_MIN = 256,
    TOKEN_OP_LOGICAL_AND = TOKEN_OP_MIN,
    TOKEN_OP_LOGICAL_OR,
    TOKEN_OP_BITWISE_AND,
    TOKEN_OP_BITWISE_OR,
    TOKEN_OP_BITWISE_XOR,
    TOKEN_OP_EQ, // ==
    TOKEN_OP_NE, // not equal
    TOKEN_OP_LT, // less than
    TOKEN_OP_GT, // greater than
    TOKEN_OP_LE, // less than or equal to
    TOKEN_OP_GE, // greater than or equal to
    TOKEN_OP_SHIFT_LEFT,
    TOKEN_OP_SHIFT_RIGHT,
    TOKEN_OP_PLUS,
    TOKEN_OP_MINUS,
    TOKEN_OP_MULTIPLY,
    TOKEN_OP_DIVIDE,
    TOKEN_OP_MODULO,
    TOKEN_OP_MAX = TOKEN_OP_MODULO,
    // string_operators relies on these being in this order so don't change this without changing that.

    // Same thing for operators applies to keywords.
    TOKEN_KEYWORD_MIN,
    TOKEN_KEYWORD_IF = TOKEN_KEYWORD_MIN,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_MAX = TOKEN_KEYWORD_ELSE,

    TOKEN_ID,

    // same thing here
    TOKEN_LITERAL_MIN,
    TOKEN_LITERAL_CHAR = TOKEN_LITERAL_MIN,
    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_DOUBLE,
    TOKEN_LITERAL_MAX = TOKEN_LITERAL_DOUBLE

} TokenType;

char *string_operators[TOKEN_OP_MAX - TOKEN_OP_MIN + 1];
int operator_precedences[TOKEN_OP_MAX - TOKEN_OP_MIN + 1];
char *string_keywords[TOKEN_KEYWORD_MAX - TOKEN_KEYWORD_MIN + 1];

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
