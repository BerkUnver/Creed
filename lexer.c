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
        .char_idx = 0,
        .peek_cached = false,
        .errors = NULL,
        .error_count = 0
    };
    return true;
}

void lexer_free(Lexer *lexer) {
    fclose(lexer->file);
    if (lexer->peek_cached) token_free(&lexer->peek);
    free(lexer->errors); // freeing a null pointer is ok
}

static int lexer_char_peek(const Lexer *lexer) {
    int c = fgetc(lexer->file);
    ungetc(c, lexer->file);
    return c;
}

static int lexer_char_get(Lexer *lexer) {
    int c = fgetc(lexer->file);
    if (c == '\n') {
        lexer->line_idx++;
        lexer->char_idx = 0;
    } else if (c != EOF) {
        lexer->char_idx++;
    }
    return c;
}

static void lexer_error_push(Lexer *lexer, LexerError error) {
    lexer->error_count++;
    lexer->errors = realloc(lexer->errors, sizeof(LexerError) * lexer->error_count); 
    lexer->errors[lexer->error_count - 1] = error;
}

// returns the literal if it is found; otherwise returns a negative number.
// length is an out-parameter that tells the length of the literal if it is found.
// If it finds a valid literal it will consume it, otherwise not.
// Currently strings and chars have the same escape sequences.
// In C, this is not true, as single quote is allowed in strings and double quote is allowed in chars.
static int lexer_literal_char_get(Lexer *lexer, int *length) {
    *length = 0;
    
    while (true) {
        int line_idx = lexer->line_idx;
        int char_idx = lexer->char_idx;
        
        int c = lexer_char_peek(lexer);

        
        if (c == '\\') { // if it is an escape sequence
            *length += 2;
            lexer_char_get(lexer);
            switch (lexer_char_get(lexer)) { // Kind of similar to how it is printed, consider consolidating.
                case '\\': return '\\';
                case 'n': return '\n';
                case 't': return '\t';
                case '0': return '\0';
                case '\'': return '\'';
                case '\"': return '\"';
                case 'r': return '\r';
                default: {
                    LexerError error = {
                        .line_idx = line_idx,
                        .char_idx = char_idx,
                        .len = 2,
                        .error = LEXER_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE
                    };
                    lexer_error_push(lexer, error);
                }
            }
        } else if (' ' <= c && c <= '~' && c != '\'' && c != '\"') { // normal character
            *length += 1;
            lexer_char_get(lexer);
            return c;
        
        } else if (c == EOF) {
            LexerError error = {
                .line_idx = line_idx,
                .char_idx = char_idx,
                .len = 1,
                .error = LEXER_ERROR_LITERAL_CHAR_EOF,
            };
            lexer_error_push(lexer, error);
            return -1;
        } else { // illegal char (could be ' or ")
            return -1;
        }
    }
}

static double lexer_decimal_get(Lexer *lexer, int *length) {
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

static Token lexer_token_get_skip_cache(Lexer *lexer) {
    // consume whitespace
    while (char_is_whitespace(lexer_char_peek(lexer))) lexer_char_get(lexer);

    Token token;
    token.line_idx = lexer->line_idx;
    token.char_idx = lexer->char_idx;

   
    if (char_is_single_char_token_type(lexer_char_peek(lexer))) {
        token.type = lexer_char_get(lexer); // chars corresponds 1:1 to unique single-char token types.
        token.len = 1;
    
    } else if (lexer_char_peek(lexer) == '0') {
        int decimal_start = lexer->char_idx;

        lexer_char_get(lexer);
        int int_peek_2 = lexer_char_peek(lexer);
        if ('0' <= int_peek_2 && int_peek_2 <= '9') { // in error state
            lexer_char_get(lexer);
            token.len = 2; // two consumed characters
            while (true) {
                int peek = lexer_char_peek(lexer);
                if ('0' <= peek && peek <= '9') {
                    lexer_char_get(lexer);
                    token.len++;
                } else if (peek == '.') {
                    LexerError error = {
                        .line_idx = lexer->line_idx,
                        .char_idx = decimal_start,
                        .len = token.len,
                        .error = LEXER_ERROR_LITERAL_DOUBLE_MULTIPLE_LEADING_ZEROS
                    };
                    lexer_error_push(lexer, error);
                    lexer_char_get(lexer);
                    
                    int decimal_length;
                    double decimal_val = lexer_decimal_get(lexer, &decimal_length);
                    token.type = TOKEN_LITERAL_DOUBLE;
                    token.data.literal_double = decimal_val;
                    token.len += 1 + decimal_length;
                    break;
                } else {
                    LexerError error = {
                        .line_idx = lexer->line_idx,
                        .char_idx = decimal_start,
                        .len = token.len,
                        .error = LEXER_ERROR_LITERAL_INT_MULTIPLE_LEADING_ZEROS
                    };
                    lexer_error_push(lexer, error);
                    token.type = TOKEN_LITERAL_INT;
                    token.data.literal_int = 0; // maybe instead use the value after the zeros
                    break;
                }
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
    
    } else if ('1' <= lexer_char_peek(lexer) && lexer_char_peek(lexer) <= '9') {
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
                break;
            }
            
            if (peek < '0' || '9' < peek) {
                token.type = TOKEN_LITERAL_INT;
                token.len = literal_int_len;
                token.data.literal_int = literal_int;
                break;
            }

            lexer_char_get(lexer);
            literal_int_len++;
            literal_int *= 10;
            literal_int += (peek - '0'); // todo: bounds-checking.
        }

    } else if (lexer_char_peek(lexer) == DELIMITER_LITERAL_CHAR) {
        
        lexer_char_get(lexer);
        int literal_length;
        int literal_char = lexer_literal_char_get(lexer, &literal_length);
        literal_length++; // opening delimiter
        
        token.type = TOKEN_LITERAL_CHAR;
        
        if (literal_char < 0) { // error condition 
            if (lexer_char_peek(lexer) == DELIMITER_LITERAL_CHAR) {
                lexer_char_get(lexer);
                literal_length++; // length of opening and closing delimiter
            }
            LexerError error = {
                .line_idx = lexer->line_idx,
                .char_idx = lexer->char_idx - literal_length, 
                .len = literal_length,
                .error = LEXER_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER 
            };
            lexer_error_push(lexer, error);
            token.data.literal_char = '@'; // random character, idk what to put here
        
        } else {
            if (lexer_char_peek(lexer) == DELIMITER_LITERAL_CHAR) {
                lexer_char_get(lexer);
                literal_length++;
            } else {  
                LexerError error = {
                    .line_idx = lexer->line_idx,
                    .char_idx = lexer->char_idx - literal_length,  
                    .len = literal_length,
                    .error = LEXER_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING
                };
                lexer_error_push(lexer, error);
            }

            token.data.literal_char = literal_char;
        }
 
        token.len = literal_length;
    
    } else if (lexer_char_peek(lexer) == DELIMITER_LITERAL_STRING) {
        lexer_char_get(lexer);
        StringBuilder builder = string_builder_new();
        token.len = 1;
        token.type = TOKEN_LITERAL_STRING;
        int literal_char_len;
        int c;
        while ((c = lexer_literal_char_get(lexer, &literal_char_len)) >= 0) {
            string_builder_add_char(&builder, c);
            token.len += literal_char_len;
        }

        token.data.literal_string = string_builder_free(&builder); 
        
        if (lexer_char_peek(lexer) == DELIMITER_LITERAL_STRING) {
            lexer_char_get(lexer);
            token.len++;
        } else {
            LexerError error = {
                .line_idx = lexer->line_idx,
                .char_idx = lexer->char_idx - token.len,
                .len = token.len,
                .error = LEXER_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING,
            };
            
            lexer_error_push(lexer, error);
        }

    } else if (char_is_operator(lexer_char_peek(lexer))) {

        char operator[OPERATOR_MAX_LENGTH + 1];
        int operator_len = 0; 
   
        while (char_is_operator(lexer_char_peek(lexer))) {
            char c = lexer_char_get(lexer);
            if (operator_len < OPERATOR_MAX_LENGTH) operator[operator_len] = c;
            operator_len++;
        }
        token.len = operator_len;
        
        if (operator_len > OPERATOR_MAX_LENGTH) {
            LexerError error = {
                .line_idx = lexer->line_idx,
                .char_idx = lexer->char_idx - operator_len,
                .len = operator_len,
                .error = LEXER_ERROR_OPERATOR_TOO_LONG
            };
            lexer_error_push(lexer, error);
            token.type = TOKEN_PLUS; // I have no idea what the value of the token should be if the operator is too long.
        } else {
            operator[operator_len] = '\0';
            if (!strcmp(operator, STR_ASSIGN)) {
                token.type = TOKEN_ASSIGN;
            } else if (!strcmp(operator, STR_EQUALS)) {
                token.type = TOKEN_EQUALS;
            } else {
                LexerError error = {
                    .line_idx = lexer->line_idx,
                    .char_idx = lexer->char_idx - token.len,
                    .len = token.len,
                    .error = LEXER_ERROR_OPERATOR_UNKNOWN
                };
                lexer_error_push(lexer, error);
                token.type = TOKEN_PLUS; // Idk what to put if the operator is unknown
            }
        }

    } else if (char_is_identifier(lexer_char_peek(lexer))) {
        StringBuilder builder = string_builder_new();
        string_builder_add_char(&builder, lexer_char_get(lexer));
        while (char_is_identifier(lexer_char_peek(lexer))) {
            string_builder_add_char(&builder, lexer_char_get(lexer));
        }
        
        token.len = builder.length; 
        char *string = string_builder_free(&builder);
        bool is_keyword = true;
        if (!strcmp(string, STR_IF)) {
            token.type = TOKEN_IF;
        } else if (!strcmp(string, STR_ELIF)) {
            token.type = TOKEN_ELIF;
        } else if (!strcmp(string, STR_ELSE)) {
            token.type = TOKEN_ELSE;
        } else {
            is_keyword = false;
            token.type =  TOKEN_ID;
            token.data.id = string;
        }

        if (is_keyword) free(string); // don't need the string because the identifier is a keyword
    
    } else {
        lexer_char_get(lexer); // consume unknown character. 
        LexerError error = {
            .line_idx = lexer->line_idx,
            .char_idx = lexer->char_idx - 1,
            .len = 1,
            .error = LEXER_ERROR_CHARACTER_UNKNOWN 
        };
        lexer_error_push(lexer, error);
        token = lexer_token_get_skip_cache(lexer);
    }
    return token;
}


TokenType lexer_token_peek_type(Lexer *lexer) {
    if (lexer->peek_cached) return lexer->peek.type;
   
    lexer->peek = lexer_token_get_skip_cache(lexer);
    lexer->peek_cached = true;

    return lexer->peek.type;
}

Token lexer_token_get(Lexer *lexer) {
    if (lexer->peek_cached) {
        lexer->peek_cached = false;
        return lexer->peek;
    }

    return lexer_token_get_skip_cache(lexer);

}
