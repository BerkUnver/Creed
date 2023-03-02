#ifndef CREED_LEXER_H
#define CREED_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include "token.h"

// treat all of these as private outside of lexer.c
typedef struct Lexer {
    FILE *file;
    
    // these tell where the lexer currently is, including peeked tokens.
    int line_idx;
    int char_idx;

    bool peek_cached;
    Token peek;
} Lexer;

bool lexer_new(const char *path, Lexer *lexer);
void lexer_free(Lexer *lexer);
Token lexer_token_get(Lexer *lexer);
TokenType lexer_token_peek_type(Lexer *lexer);

#endif
