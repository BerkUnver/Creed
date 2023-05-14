#ifndef CREED_LEXER_H
#define CREED_LEXER_H

#include <stdio.h>
#include <stdbool.h>
#include "token.h"

#define LEXER_TOKEN_PEEK_MAX 3
typedef struct Lexer {
    StringId file_name;
    StringId file_content;
    const char *file_content_ptr; // We keep a raw pointer to this so we can access it quickly.
    int idx_char;
    int idx_line;

    int peek_count;
    int peek_idx;
    Token peeks[LEXER_TOKEN_PEEK_MAX];
} Lexer;

Lexer lexer_new(StringId path);

void lexer_free(Lexer *lexer);
Token lexer_token_get(Lexer *lexer);
Token lexer_token_peek(Lexer *lexer);
Token lexer_token_peek_many(Lexer *lexer, int count);
#endif

