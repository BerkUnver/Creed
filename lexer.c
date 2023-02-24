#include <stdio.h>
#include <string.h>
#include "token.h"
#include "lexer.h"

bool lexer_new(const char *path, Lexer *lexer) {
    FILE *file = fopen(path, "r+");
    if (!file) return false;
    *lexer = (Lexer) {
        .file = file,
        .line_idx = 0,
        .char_idx = 0
    };
    return true;
}

void lexer_free(Lexer *lexer) {
    fclose(lexer->file);
}

int lexer_char_peek(const Lexer *lexer) {
    int c = fgetc(lexer->file);
    ungetc(c, lexer->file);
    return c;
}

int lexer_char_get(Lexer *lexer) {
    int c = fgetc(lexer->file);
    if (c == '\n') {
        lexer->line_idx++;
        lexer->char_idx = 0;
    } else {
        lexer->char_idx++;
    }
    return c;
}

Token lexer_token_get(Lexer *lexer) {
    int c; // c always is the next peeked character of the lexer.
    
    // consume whitespace
    while (true) {
        c = lexer_char_peek(lexer);
        if (!char_is_whitespace(c)) break;
        lexer_char_get(lexer);
    }

    Token token;
    token.line_idx = lexer->line_idx;
    token.char_idx = lexer->char_idx;

   
    // check if token is a single char
    if (char_is_single_char_token_type(c)) {
        lexer_char_get(lexer);
        token.type = c; // This is ok because c corresponds 1:1 to unique single-char token types.
        token.len = 1;
        return token;
    }

    // checking if character is operator.
    char operator[OPERATOR_MAX_LENGTH + 1];
    int operator_len = 0; 
    
    while (char_is_operator(c) && operator_len < OPERATOR_MAX_LENGTH) { 
        operator[operator_len] = c;
        lexer_char_get(lexer);
        c = lexer_char_peek(lexer);
        operator_len++;
    }

    operator[operator_len] = '\0'; 
    if (operator_len > 0) { // if the first character is an operator, meaning the whole string is an operator
        token.len = operator_len;
        
        if (operator_len > OPERATOR_MAX_LENGTH) {
            token.type = TOKEN_ERROR;
            token.data.error = "No operator is this long.";
        } else if (!strcmp(operator, STR_ASSIGN)) {
            token.type = TOKEN_ASSIGN;
        } else if (!strcmp(operator, STR_EQUALS)) {
            token.type = TOKEN_EQUALS;
        } else {
            token.type = TOKEN_ERROR;
            token.data.error = "Unrecognized operator.";
        }     
        return token;
    }
    
    
    // check if character is identifier, similar to above.
    char id[ID_MAX_LENGTH + 1];
    int id_len = 0;

    while (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') && id_len < ID_MAX_LENGTH) { 
        id[id_len] = c;
        lexer_char_get(lexer);
        c = lexer_char_peek(lexer);
        id_len++; 
    }

    id[id_len] = '\0';
    
    if (id_len > 0) {
        token.len = id_len;
        if (id_len > ID_MAX_LENGTH) {
            token.type = TOKEN_ERROR;
            token.data.error = "Identifier is too long."; // todo: Somehow statically bake token length into the identifier.
        } else if (!strcmp(id, STR_IF)) {
            token.type = TOKEN_IF;
        } else if (!strcmp(id, STR_ELIF)) {
            token.type = TOKEN_ELIF;
        } else if (!strcmp(id, STR_ELSE)) {
            token.type = TOKEN_ELSE;
        } else {
            token.type = TOKEN_ERROR;
            token.data.error = "Unrecognized identifier.";
        }
        return token;
    }

    lexer_char_get(lexer); 
    token.type = TOKEN_ERROR;
    token.len = 1;
    token.data.error = "Unrecognized character.";
    return token;
}
