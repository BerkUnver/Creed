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
    }
    return c;
}

Token lexer_token_get(Lexer *lexer) {
    int c;
    
    // consume whitespace
    while (true) {
        c = lexer_char_peek(lexer);
        bool is_whitespace = false;
        for (int i = 0; i < sizeof(whitespace_chars); i++) {
            if (whitespace_chars[i] != c) continue;
            is_whitespace = true;
            break;
        }
        if (!is_whitespace) break;
        lexer_char_get(lexer);
    }

    Token token;
    token.line_idx = lexer->line_idx;
    token.char_idx = lexer->char_idx;

   
    // check if token is a single char
    for (int i = 0; i < sizeof(single_char_tokens); i++) {
        if (c != single_char_tokens[i]) continue;
        lexer_char_get(lexer);
        token.type = c; // ok because chars correspond 1:1 to token type.
        token.len = 1;
        return token;
    }
    
    // checking if character is operator.
    char operator[OPERATOR_MAX_LENGTH + 1];
    int operator_len = 0; 
    
    while (operator_len < OPERATOR_MAX_LENGTH) {
        bool is_operator = false;
        for (int i = 0; i < sizeof(operator_chars); i++) {
            if (c != operator_chars[i]) continue;
            operator[operator_len] = c;
            lexer_char_get(lexer);
            is_operator = true;
            break;
        }
        
        if (!is_operator) break;

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

    while (id_len < ID_MAX_LENGTH) { 
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) break;
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
    printf("\n%c\n", c);
    token.data.error = "Unrecognized character.";
    return token;
}
