#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "print.h"
#include "string_builder.h"
#include "token.h"

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
// length is an out-parameter that tells the length of the literal if it is found.
// If it finds a valid literal it will consume it, otherwise not.
// Currently strings and chars have the same escape sequences.
// In C, this is not true, as single quote is allowed in strings and double quote is allowed in chars.
int lexer_literal_char_try_get(Lexer *lexer, int *length) {
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
    if (c < ' ' || c > '~' || c == '\'' || c == '\"') return -1; // outside the range of valid non-escape literals.
    *length = 1;
    lexer_char_get(lexer);
    return c;
}

double lexer_decimal_get(Lexer *lexer, int *length) {
    double val = 0;
    double digit = 0.1;
    *length = 0;
    while (true) {
        int peek = lexer_char_peek(lexer);
        if (peek < '0' || '9' < peek) return val; 
        lexer_char_get(lexer);
        val += (digit * (peek - '0'));
        digit /= 10;
        (*length)++;
    }
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
    
    if (lexer_char_peek(lexer) == '0') {
        lexer_char_get(lexer);
        int int_peek_2 = lexer_char_peek(lexer);
        if ('0' <= int_peek_2 && int_peek_2 <= '9') {
            lexer_char_get(lexer);
            token.type = TOKEN_ERROR;
            token.len = 2; // two consumed characters
            while (true) {
                int peek = lexer_char_peek(lexer);
                if (peek == '.') {
                    lexer_char_get(lexer);
                    int decimal_length;
                    lexer_decimal_get(lexer, &decimal_length);
                    token.len += 1 + decimal_length;
                    token.data.error = "Decimal literals cannot have more than 1 zero before the decimal point.";
                    break;
                }
                if (peek < '0' || '9' < peek) {
                    token.data.error = "Non-zero integer literals cannot start with 0.";
                    break;
                }
                lexer_char_get(lexer);
                token.len++;
            }
        } else if (int_peek_2 == '.') {
            lexer_char_get(lexer);
            token.type = TOKEN_LITERAL_DOUBLE;
            token.data.literal_double = lexer_decimal_get(lexer, &token.len);
            token.len += 2; // length of "0." and length of decimal
        } else {
            token.type = TOKEN_LITERAL_INT;
            token.data.literal_int = 0;
            token.len = 1;
        }
        return token;
    }

    if ('1' <= lexer_char_peek(lexer) && lexer_char_peek(lexer) <= '9') {
        int literal_int = lexer_char_get(lexer) - '0';
        int literal_int_len = 1;
        while (true) {
            int peek = lexer_char_peek(lexer);
            
            if (peek == '.') {
                lexer_char_get(lexer);
                double literal_double = (double) literal_int;
                int decimal_len;
                literal_double += lexer_decimal_get(lexer, &decimal_len);
                token.type = TOKEN_LITERAL_DOUBLE;
                token.len = literal_int_len + 1 + decimal_len;
                token.data.literal_double = literal_double;
                return token;
            }
            
            if (peek < '0' || '9' < peek) {
                token.type = TOKEN_LITERAL_INT;
                token.len = literal_int_len;
                token.data.literal_int = literal_int;
                return token;
            }

            lexer_char_get(lexer);
            literal_int_len++;
            literal_int *= 10;
            literal_int += (peek - '0'); // todo: bounds-checking.
        }
    }
    
    // this is bad. I (Berk) make alot of assumptions about how we want to handle errors that are probably bad.
    if (lexer_char_peek(lexer) == DELIMITER_LITERAL_CHAR) {
        lexer_char_get(lexer);
        int literal_length;
        int literal_char = lexer_literal_char_try_get(lexer, &literal_length);
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

    if (lexer_char_peek(lexer) == DELIMITER_LITERAL_STRING) {
        lexer_char_get(lexer);
        StringBuilder builder = string_builder_new();
        token.len = 2;
        int literal_char_len;
        int c;
        while ((c = lexer_literal_char_try_get(lexer, &literal_char_len)) >= 0) {
            string_builder_add_char(&builder, c);
            token.len += literal_char_len;
        }

        char *string = string_builder_free(&builder); 
        if (lexer_char_get(lexer) != DELIMITER_LITERAL_STRING) {
            token.type = TOKEN_ERROR;
            token.data.error = "Expected a valid character or the end delimiter \" at the end of a literal string.";
            free(string);
        } else {
            token.type = TOKEN_LITERAL_STRING;
            token.data.literal_string = string;
        }

        return token;
    }

    // checking if character is operator.
    char operator[OPERATOR_MAX_LENGTH + 1];
    int operator_len = 0; 
   
    while (char_is_operator(lexer_char_peek(lexer))) {
        char c = lexer_char_get(lexer);
        if (operator_len < OPERATOR_MAX_LENGTH) operator[operator_len] = c;
        operator_len++;
    }

    if (operator_len > 0) { // if the first character is an operator, meaning the whole string is an operator
        token.len = operator_len;
        
        if (operator_len > OPERATOR_MAX_LENGTH) {
            token.type = TOKEN_ERROR;
            token.data.error = "No operator is this long.";
            return token;
        }
        
        operator[operator_len] = '\0';
        if (!strcmp(operator, STR_ASSIGN)) {
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
    // I (Berk) decided to use a static array instead of a StringBuffer.
    // This is so we don't have to malloc a string array when the identifier is a keyword.
    // Requires us to put a identifier maximum length so we don't overflow the buffer
    char id[ID_MAX_LENGTH + 1];
    int id_len = 0;

    while (char_is_identifier(lexer_char_peek(lexer))) { 
        char c = lexer_char_get(lexer);
        if (id_len < ID_MAX_LENGTH) id[id_len] = c;
        id_len++; 
    }

    if (id_len > 0) {
        token.len = id_len;
        if (id_len > ID_MAX_LENGTH) {
            token.type = TOKEN_ERROR;
            token.data.error = "Identifier is too long."; // todo: Somehow statically bake max identifier length into the error string.
            return token;
        }

        id[id_len] = '\0';
        if (!strcmp(id, STR_IF)) {
            token.type = TOKEN_IF;
        } else if (!strcmp(id, STR_ELIF)) {
            token.type = TOKEN_ELIF;
        } else if (!strcmp(id, STR_ELSE)) {
            token.type = TOKEN_ELSE;
        } else {
            char *id_copy = malloc(sizeof(char) * (id_len + 1));
            strcpy(id_copy, id);

            token.type = TOKEN_ID;
            token.data.id = id_copy;
        }
        return token;
    }

    lexer_char_get(lexer); // consume unknown character. 
    token.type = TOKEN_ERROR;
    token.len = 1;
    token.data.error = "Unrecognized character.";
    return token;
}
