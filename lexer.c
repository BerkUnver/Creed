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

// peeks count characters ahead. Rounds anything lower than 1 to 1.
int lexer_chars_peek(const Lexer *lexer, int count) {
    int pushback = fgetc(lexer->file);
    int c = count > 1 ? lexer_chars_peek(lexer, count - 1) : pushback;
    ungetc(pushback, lexer->file);
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

// returns the literal if it is found; otherwise returns a negative number.
// length is an out-parameter that tells the length of the literal.
int lexer_literal_char_get(Lexer *lexer, int *length) {
    int c = lexer_char_peek(lexer);
    if (c == '\\') { // if it is an escape sequence
        int escape; 
        switch (lexer_chars_peek(lexer, 2)) { // Kind of similar to how it is printed, consider consolidating.
            case '\\': escape = '\\'; break;
            case 'n': escape = '\n'; break;
            case 't': escape = '\t'; break;
            case '0': escape = '\0'; break;
            case '\'': escape = '\''; break;
            case '\"': escape = '\"'; break;
            case 'r': escape = '\r'; break;
            default: return -1; // not a valid escape sequence
        }
        lexer_char_get(lexer);
        lexer_char_get(lexer);
        *length = 2;
        return escape;
    }
    if (c < ' ' || c > '~') return -1; // outside the range of valid non-escape literals.
    *length = 1;
    lexer_char_get(lexer);
    return c;
}

Token lexer_token_get(Lexer *lexer) {
    // consume whitespace
    while (char_is_whitespace(lexer_char_peek(lexer))) lexer_char_get(lexer);

    Token token;
    token.line_idx = lexer->line_idx;
    token.char_idx = lexer->char_idx;

   
    if (char_is_single_char_token_type(lexer_char_peek(lexer))) {
        token.type = lexer_char_get(lexer); // chars corresponds 1:1 to unique single-char token types.
        token.len = 1;
        return token;
    }

    // This is bad. I (Berk) make alot of assumptions about how we want to handle errors that are probably bad.
    if (lexer_char_peek(lexer) == DELIMITER_LITERAL_CHAR) {
        lexer_char_get(lexer);
        int literal_length;
        int literal_char = lexer_literal_char_get(lexer, &literal_length);
        if (literal_char < 0) {
            lexer_char_get(lexer);
            token.type = TOKEN_ERROR;
            token.data.error = "Invalid character literal.";
            token.len = 2; // opening delimiter + character length.
            lexer_char_get(lexer);
            return token;
        }
        
        if (lexer_char_get(lexer) != DELIMITER_LITERAL_CHAR) {
            token.type = TOKEN_ERROR;
            token.data.error = "Expected \' at the end of a character literal.";
        } else {
            token.type = TOKEN_LITERAL_CHAR;
            token.data.literal_char = literal_char;
        }

        token.len = literal_length + 2; // add opening and closing delimiters.
        return token;
    }

    // checking if character is operator.
    char operator[OPERATOR_MAX_LENGTH + 1];
    int operator_len = 0; 
    
    while (char_is_operator(lexer_char_peek(lexer)) && operator_len < OPERATOR_MAX_LENGTH) { 
        operator[operator_len] = lexer_char_get(lexer);
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

    while (char_is_identifier(lexer_char_peek(lexer)) && id_len < ID_MAX_LENGTH) { 
        id[id_len] = lexer_char_get(lexer);
        id_len++; 
    }

    id[id_len] = '\0';
    
    if (id_len > 0) {
        token.len = id_len;
        if (id_len >= ID_MAX_LENGTH) {
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

    lexer_char_get(lexer); // consume unknown character. 
    token.type = TOKEN_ERROR;
    token.len = 1;
    token.data.error = "Unrecognized character.";
    return token;
}
