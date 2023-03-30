#ifndef CREED_TOKEN_H
#define CREED_TOKEN_H

#include <stdio.h>
#include <stdbool.h>
#include "string_cache.h"

#define ID_MAX_LENGTH 64
#define OPERATOR_MAX_LENGTH 2

#define STR_ASSIGN "="

#define DELIMITER_LITERAL_STRING '"'
#define DELIMITER_LITERAL_CHAR '\''

#define NUM_INT_TYPE_SPECS 8
#define NUM_FLOAT_TYPE_SPECS 2

typedef struct Location {
    int line_start;
    int line_end;
    int char_start;
    int char_end;
} Location;

Location location_expand(Location begin, Location end);
void location_print(Location location);

typedef struct Literal {
    enum {
        LITERAL_STRING,
        LITERAL_INT_MIN,
        LITERAL_INT8 = LITERAL_INT_MIN,
        LITERAL_INT16,
        LITERAL_INT,
        LITERAL_INT64,
        LITERAL_UINT8,
        LITERAL_UINT16,
        LITERAL_UINT,
        LITERAL_UINT64,
        LITERAL_FLOAT_MIN,
        LITERAL_FLOAT = LITERAL_FLOAT_MIN,
        LITERAL_FLOAT64,
        LITERAL_CHAR
    } type;

    union {
        StringId l_string;
        char l_int8;
        short l_int16;
        int l_int;
        long l_int64;
        unsigned char l_uint8;
        unsigned short l_uint16;
        unsigned int l_uint;
        unsigned long l_uint64;
        float l_float;
        double l_float64;
        char l_char;
    } data;
} Literal; 

void literal_print(Literal *literal);

typedef enum TokenType {
    TOKEN_EOF = EOF,

    TOKEN_PAREN_OPEN = '(',
    TOKEN_PAREN_CLOSE = ')',
    TOKEN_CURLY_BRACE_OPEN = '{',
    TOKEN_CURLY_BRACE_CLOSE = '}',
    TOKEN_BRACKET_OPEN = '[',
    TOKEN_BRACKET_CLOSE = ']',
    
    TOKEN_BITWISE_NOT = '~',
    TOKEN_QUESTION_MARK = '?',
    TOKEN_COLON = ':',
    TOKEN_COMMA = ',',
    TOKEN_SEMICOLON = ';',
    TOKEN_DOT = '.', 
    // when this appears in a literal float, we consume it as part of the float.
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

    TOKEN_UNARY_LOGICAL_NOT,
    
    TOKEN_INCREMENT,
    TOKEN_DEINCREMENT,

    TOKEN_ASSIGN_MIN,
    TOKEN_ASSIGN = TOKEN_ASSIGN_MIN,
    TOKEN_ASSIGN_LOGICAL_AND,
    TOKEN_ASSIGN_LOGICAL_OR,
    TOKEN_ASSIGN_BITWISE_AND,
    TOKEN_ASSIGN_BITWISE_OR,
    TOKEN_ASSIGN_BITWISE_XOR,
    TOKEN_ASSIGN_SHIFT_LEFT,
    TOKEN_ASSIGN_SHIFT_RIGHT,
    TOKEN_ASSIGN_PLUS,
    TOKEN_ASSIGN_MINUS,
    TOKEN_ASSIGN_MULTIPLY,
    TOKEN_ASSIGN_DIVIDE,
    TOKEN_ASSIGN_MODULO,
    TOKEN_ASSIGN_MAX = TOKEN_ASSIGN_MODULO,
    // string_assigns relies on this order so don't change.

    // Same thing for operators applies to keywords.
    TOKEN_KEYWORD_MIN,
    TOKEN_KEYWORD_IF = TOKEN_KEYWORD_MIN,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_TYPECAST,          // "as"
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_WHILE,  
    TOKEN_KEYWORD_IN,               // "for var in arr[]"
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_CONST,
    TOKEN_KEYWORD_VOID,
    TOKEN_KEYWORD_CHAR,
    TOKEN_KEYWORD_INT8,
    TOKEN_KEYWORD_INT16,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_INT64,
    TOKEN_KEYWORD_UINT8,
    TOKEN_KEYWORD_UINT16,
    TOKEN_KEYWORD_UINT,
    TOKEN_KEYWORD_UINT64,
    TOKEN_KEYWORD_FLOAT,
    TOKEN_KEYWORD_FLOAT64,
    TOKEN_KEYWORD_BOOL,
    TOKEN_KEYWORD_FILE,
    TOKEN_KEYWORD_REGEX,
    TOKEN_KEYWORD_ENUM,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_UNION,
    TOKEN_KEYWORD_SUM,
    TOKEN_KEYWORD_MATCH,
    TOKEN_KEYWORD_GOTO,
    TOKEN_KEYWORD_LABEL,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_MAX = TOKEN_KEYWORD_RETURN,

    TOKEN_ID,
    TOKEN_LITERAL,

    TOKEN_ERROR_MIN,
    TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE,
    TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER,
    TOKEN_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING,
    TOKEN_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING,
    TOKEN_ERROR_LITERAL_NUMBER_LEADING_ZERO,
    TOKEN_ERROR_LITERAL_NUMBER_ILLEGAL_TYPE_SPEC,
    TOKEN_ERROR_OPERATOR_TOO_LONG,
    TOKEN_ERROR_OPERATOR_UNKNOWN,
    TOKEN_ERROR_CHARACTER_UNKNOWN,
    TOKEN_ERROR_MAX = TOKEN_ERROR_CHARACTER_UNKNOWN
} TokenType;

char *string_operators[TOKEN_OP_MAX - TOKEN_OP_MIN + 1];
int operator_precedences[TOKEN_OP_MAX - TOKEN_OP_MIN + 1];
char *string_keywords[TOKEN_KEYWORD_MAX - TOKEN_KEYWORD_MIN + 1];
char *string_assigns[TOKEN_ASSIGN_MAX - TOKEN_ASSIGN_MIN + 1];

char *int_type_specs[NUM_INT_TYPE_SPECS];
char *float_type_specs[NUM_FLOAT_TYPE_SPECS];

typedef union TokenData {
    Literal literal;
    StringId id;
} TokenData;

typedef struct Token {
    TokenType type;
    TokenData data;
    Location location;
} Token;

// c is an int so you can also pass a TokenType itself in here instead of a char.
bool char_is_single_char_token_type(int c);

bool char_is_whitespace(char c);

void token_print(Token *token);
char *token_free_get_id(Token *token);
Literal token_free_get_literal(Token *token);
#endif
