#ifndef CREED_LEXER_H
#define CREED_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include "token.h"


typedef enum LexerErrorCode {
    LEXER_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE,
    LEXER_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER,
    LEXER_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING,
    LEXER_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING,
    LEXER_ERROR_LITERAL_DOUBLE_MULTIPLE_LEADING_ZEROS,
    LEXER_ERROR_LITERAL_INT_MULTIPLE_LEADING_ZEROS,
    LEXER_ERROR_OPERATOR_TOO_LONG,
    LEXER_ERROR_OPERATOR_UNKNOWN,
    LEXER_ERROR_CHARACTER_UNKNOWN
} LexerErrorCode;

typedef struct LexerError {
    int line_idx;
    int char_idx;
    int len;
    LexerErrorCode code;
} LexerError;

typedef struct Lexer {
    FILE *file;
    
    // these tell where the lexer currently is, including peeked tokens.
    int line_idx;
    int char_idx;

    bool peek_cached;
    Token peek;

    LexerError *errors;
    int error_count;
} Lexer;

bool lexer_new(const char *path, Lexer *lexer);
void lexer_free(Lexer *lexer);
Token lexer_token_get(Lexer *lexer);
TokenType lexer_token_peek_type(Lexer *lexer);

#endif
