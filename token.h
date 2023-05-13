#ifndef CREED_TOKEN_H
#define CREED_TOKEN_H

#include <stdio.h>
#include <stdbool.h>
#include "prelude.h"
#include "string_cache.h"

#define ID_MAX_LENGTH 64
#define OPERATOR_MAX_LENGTH 2

#define DELIMITER_LITERAL_STRING '"'
#define DELIMITER_LITERAL_CHAR '\''

typedef enum TokenType {
    TOKEN_NULL = '\0',

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
    
    TOKEN_LAMBDA,
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
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_TYPECAST,          // "as"
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_WHILE,  
    TOKEN_KEYWORD_IN,               // "for var in arr[]"
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_TYPE_MIN,
    TOKEN_KEYWORD_TYPE_VOID = TOKEN_KEYWORD_TYPE_MIN,
    TOKEN_KEYWORD_TYPE_CHAR, // Literal enum ordering depends on the ordering of the following.
    TOKEN_KEYWORD_TYPE_NUMERIC_MIN,
    TOKEN_KEYWORD_TYPE_INTEGER_MIN = TOKEN_KEYWORD_TYPE_NUMERIC_MIN,
    TOKEN_KEYWORD_TYPE_INT8 = TOKEN_KEYWORD_TYPE_INTEGER_MIN,
    TOKEN_KEYWORD_TYPE_INT16,
    TOKEN_KEYWORD_TYPE_INT,
    TOKEN_KEYWORD_TYPE_INT64,
    TOKEN_KEYWORD_TYPE_UINT_MIN,
    TOKEN_KEYWORD_TYPE_UINT8 = TOKEN_KEYWORD_TYPE_UINT_MIN,
    TOKEN_KEYWORD_TYPE_UINT16,
    TOKEN_KEYWORD_TYPE_UINT,
    TOKEN_KEYWORD_TYPE_UINT64,
    TOKEN_KEYWORD_TYPE_UINT_MAX = TOKEN_KEYWORD_TYPE_UINT64,
    TOKEN_KEYWORD_TYPE_INTEGER_MAX = TOKEN_KEYWORD_TYPE_UINT64,
    TOKEN_KEYWORD_TYPE_FLOAT,
    TOKEN_KEYWORD_TYPE_FLOAT64,
    TOKEN_KEYWORD_TYPE_NUMERIC_MAX = TOKEN_KEYWORD_TYPE_FLOAT64,
    TOKEN_KEYWORD_TYPE_BOOL,
    TOKEN_KEYWORD_TYPE_MAX = TOKEN_KEYWORD_TYPE_BOOL,
    
    TOKEN_KEYWORD_FALSE,
    TOKEN_KEYWORD_TRUE,

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
    TOKEN_KEYWORD_IMPORT,
    TOKEN_KEYWORD_MAX = TOKEN_KEYWORD_IMPORT,

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
