#ifndef CREED_LEXER_H
#define CREED_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include "token.h"

typedef struct Lexer {
    FILE *file;
    
    // these tell where the lexer currently is, including peeked tokens.
    int line_idx;
    int char_idx;

    int peeks;
    Token peek1;
    Token peek2;
} Lexer;

Lexer lexer_new(const char *path);
void lexer_free(Lexer *lexer);
Token lexer_token_get(Lexer *lexer);
Token lexer_token_peek(Lexer *lexer);
Token lexer_token_peek_2(Lexer *lexer);
#endif
