#ifndef CREED_LEXER_H
#define CREED_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include "token.h"

typedef struct Lexer {
    StringId file_name;
    StringId file_content;
    const char *file_content_ptr; // We keep a raw pointer to this so we can access it quickly.
    int idx_char;
    int idx_line;

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
