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

    bool peek_cached;
    Token peek;
} Lexer;

bool lexer_new(const char *path, Lexer *lexer);
void lexer_free(Lexer *lexer);
Token lexer_token_get(Lexer *lexer);
Token *lexer_token_peek(Lexer *lexer); // do not let this pointer live after this function call.
#endif
