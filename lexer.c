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

static int fpeek(FILE *file) {
    int c = fgetc(file);
    ungetc(c, file);
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

// the starting index of the error is determined by doing lexer->char_idx - len.
void lexer_error_push(Lexer *lexer, Token *token, LexerErrorCode code) {
    lexer->error_count++;
    lexer->errors = realloc(lexer->errors, sizeof(LexerError) * lexer->error_count); 
    lexer->errors[lexer->error_count - 1] = (LexerError) {
        .line_idx = token->line_idx,
        .char_idx = token->char_idx,
        .len = token->len,
        .code = code
    };
}

// returns the literal if it is found; otherwise returns a negative number.
// length is an out-parameter that is incremented by the length of the literal if it is found.
// If it finds a valid literal it will consume it, otherwise not.
// Currently strings and chars have the same escape sequences.
// In C, this is not true, as single quote is allowed in strings and double quote is allowed in chars.
static int lexer_literal_char_get(Lexer *lexer, int *length) {
    while (true) {
        
        int c = fpeek(lexer->file);
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
                    // this is a hack.
                    Token token_error = {
                        .line_idx = lexer->line_idx,
                        .char_idx = lexer->char_idx - 2,
                        .len = 2
                    };
                    lexer_error_push(lexer, &token_error, LEXER_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE);
                }
            }
        } else if (' ' <= c && c <= '~' && c != '\'' && c != '\"') { // normal character
            (*length)++;
            lexer_char_get(lexer);
            return c;
        } else { // illegal char (could be ' or ")
            return -1;
        }
    }
}

// length is INCREMENTED (not assigned) to length of decimal.
static double lexer_decimal_get(Lexer *lexer, int *length) {
    double val = 0;
    double digit = 0.1;
    while (true) {
        int peek = fpeek(lexer->file);
        if (peek < '0' || '9' < peek) return val; 
        lexer_char_get(lexer);
        val += (digit * (peek - '0'));
        digit /= 10;
        (*length)++;
    }
}

static Token lexer_token_get_skip_cache(Lexer *lexer) {
    // consume whitespace
    while (char_is_whitespace(fpeek(lexer->file))) lexer_char_get(lexer);

    Token token;
    token.line_idx = lexer->line_idx;
    token.char_idx = lexer->char_idx;

    int peek = fpeek(lexer->file); 
    if (char_is_single_char_token_type(peek)) {
        token.type = lexer_char_get(lexer); // chars corresponds 1:1 to unique single-char token types.
        token.len = 1;
    
    } else if (peek == '0') { // get decimals that start with 0.
        lexer_char_get(lexer);
        int int_peek_2 = fpeek(lexer->file);
        if ('0' <= int_peek_2 && int_peek_2 <= '9') { // in error state
            lexer_char_get(lexer);
            token.len = 2; // two consumed characters
            while (true) {
                int digit = fpeek(lexer->file);
                if ('0' <= digit && digit <= '9') {
                    lexer_char_get(lexer);
                    token.len++;
                } else break;
            }
            
            if (fpeek(lexer->file) == '.') {
                lexer_char_get(lexer);
                token.len++;

                double decimal_val = lexer_decimal_get(lexer, &token.len);
                token.type = TOKEN_LITERAL_DOUBLE;
                token.data.literal_double = decimal_val;
            
                lexer_error_push(lexer, &token, LEXER_ERROR_LITERAL_DOUBLE_MULTIPLE_LEADING_ZEROS);

            } else {
                token.type = TOKEN_LITERAL_INT;
                token.data.literal_int = 0; // maybe instead use the value after the zeros
            
                lexer_error_push(lexer, &token, LEXER_ERROR_LITERAL_INT_MULTIPLE_LEADING_ZEROS);
            }
        } else if (int_peek_2 == '.') {
            lexer_char_get(lexer);
            token.type = TOKEN_LITERAL_DOUBLE;
            token.len = 2;
            token.data.literal_double = lexer_decimal_get(lexer, &token.len);
        } else {
            token.type = TOKEN_LITERAL_INT;
            token.data.literal_int = 0;
            token.len = 1;
        }
    
    } else if ('1' <= peek && peek <= '9') {
        token.len = 0; 
        int literal_int = lexer_char_get(lexer) - '0';
        while (true) {
            int digit = fpeek(lexer->file);
            if (digit < '0' || '9' < digit) break;
            lexer_char_get(lexer);
            token.len++;
            literal_int *= 10;
            literal_int += (digit - '0'); // todo: bounds-checking.
        }

        if (fpeek(lexer->file) == '.') {
            lexer_char_get(lexer);
            token.len++;
            token.type = TOKEN_LITERAL_DOUBLE;
            token.data.literal_double = ((double) literal_int) + lexer_decimal_get(lexer, &token.len);
        
        } else {
            token.type = TOKEN_LITERAL_INT;
            token.data.literal_int = literal_int;
        }   

    } else if (peek == DELIMITER_LITERAL_CHAR) {
        
        lexer_char_get(lexer);
        token.len = 1;
        int literal_char = lexer_literal_char_get(lexer, &token.len);
        
        token.type = TOKEN_LITERAL_CHAR;
        
        if (literal_char < 0) { // error condition 
            if (fpeek(lexer->file) == DELIMITER_LITERAL_CHAR) {
                lexer_char_get(lexer);
                token.len++; // length of closing delimiter
            }

            token.data.literal_char = '@'; // random character, idk what to put here
            lexer_error_push(lexer, &token, LEXER_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER); 

        } else {
            token.data.literal_char = literal_char;
            if (fpeek(lexer->file) == DELIMITER_LITERAL_CHAR) {
                lexer_char_get(lexer);
                token.len++;
            } else {
                lexer_error_push(lexer, &token, LEXER_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING);
            }
        }
    } else if (peek == DELIMITER_LITERAL_STRING) {
        lexer_char_get(lexer);
        StringBuilder builder = string_builder_new();
        token.len = 1;
        token.type = TOKEN_LITERAL_STRING;
        
        int c;
        while ((c = lexer_literal_char_get(lexer, &token.len)) >= 0) {
            string_builder_add_char(&builder, c);
        }

        token.data.literal_string = string_builder_free(&builder); 
        
        if (fpeek(lexer->file) == DELIMITER_LITERAL_STRING) {
            lexer_char_get(lexer);
            token.len++;
        } else {
            lexer_error_push(lexer, &token, LEXER_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING);
        }

    } else if (char_is_operator(peek)) {

        char operator[OPERATOR_MAX_LENGTH + 1];
        token.len = 0; 
   
        while (char_is_operator(fpeek(lexer->file))) {
            char c = lexer_char_get(lexer);
            if (token.len < OPERATOR_MAX_LENGTH) operator[token.len] = c;
            token.len++;
        }
        
        if (token.len > OPERATOR_MAX_LENGTH) {
            token.type = TOKEN_OP_PLUS; // I have no idea what the value of the token should be if the operator is too long.
            lexer_error_push(lexer, &token, LEXER_ERROR_OPERATOR_TOO_LONG);
        } else {
            operator[token.len] = '\0';
            bool is_operator = false;
            for (int i = 0; i < sizeof(string_operators) / sizeof(*string_operators); i++) {
                if (!strcmp(operator, string_operators[i])) {
                    token.type = TOKEN_OP_MIN + i;
                    is_operator = true;
                    break;
                }
            }

            if (!is_operator) { 
                lexer_error_push(lexer, &token, LEXER_ERROR_OPERATOR_UNKNOWN);
                token = lexer_token_get_skip_cache(lexer);
                // potentially want better error-handling behavior here, i.e. insert a dummy operator where the malformed one is.
            }
        }

    } else if (char_is_identifier(peek)) {
        StringBuilder builder = string_builder_new();
        string_builder_add_char(&builder, lexer_char_get(lexer));
        while (char_is_identifier(fpeek(lexer->file))) {
            string_builder_add_char(&builder, lexer_char_get(lexer));
        }
        
        token.len = builder.length; 
        char *string = string_builder_free(&builder);
        bool is_keyword = false;
        for (int i = 0; i < sizeof(string_keywords) / sizeof(*string_keywords); i++) {
            if (!strcmp(string, string_keywords[i])) {
                token.type = TOKEN_KEYWORD_MIN + i;
                is_keyword = true;
                free(string);
                break;
            }
        }

        if (!is_keyword) {
            token.type = TOKEN_ID;
            token.data.id = string;
        }
    } else {
        lexer_char_get(lexer);
        lexer_error_push(lexer, &token, LEXER_ERROR_CHARACTER_UNKNOWN);
        token = lexer_token_get_skip_cache(lexer);
    }
    
    return token;
}

Token *lexer_token_peek(Lexer *lexer) {
    if (!lexer->peek_cached) {
        lexer->peek = lexer_token_get_skip_cache(lexer);
        lexer->peek_cached = true;
    }
    return &lexer->peek;
}

Token lexer_token_get(Lexer *lexer) {
    if (lexer->peek_cached) {
        lexer->peek_cached = false;
        return lexer->peek;
    }
    return lexer_token_get_skip_cache(lexer);
}

void lexer_error_print(Lexer *lexer) {
    for (int i = 0; i < lexer->error_count; i++) {
        LexerError err = lexer->errors[i];
        printf("[Line: %i, char: %i, len: %i]: ", err.line_idx + 1, err.char_idx + 1, err.len);
        switch (err.code) {
            case LEXER_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE:
                puts("This is not a valid character escape sequence.");
                break;
            case LEXER_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER:
                puts("This is not a valid character literal.");
                break;
            case LEXER_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING:
                printf("A character literal must end with %c.\n", DELIMITER_LITERAL_CHAR);
                break;
            case LEXER_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING:
                printf("A string literal must end with %c.\n", DELIMITER_LITERAL_STRING);
                break;
            case LEXER_ERROR_LITERAL_DOUBLE_MULTIPLE_LEADING_ZEROS:
                puts("A literal double cannot have multiple leading zeros.");
                break;
            case LEXER_ERROR_LITERAL_INT_MULTIPLE_LEADING_ZEROS:
                puts("A literal int cannot have multiple leading zeros.");
                break;
            case LEXER_ERROR_OPERATOR_TOO_LONG:
                printf("No existing operator exceeds the length %i.\n", OPERATOR_MAX_LENGTH);
                break;
            case LEXER_ERROR_OPERATOR_UNKNOWN:
                puts("This operator is unknown.");
                break;
            case LEXER_ERROR_CHARACTER_UNKNOWN:
                puts("This character is not allowed in source files.");
                break;
            case LEXER_ERROR_EXPECTED_PAREN_CLOSE:
                puts("Expected a closing parenthesis to match the opening parenthesis of an expression.");
                break;
            case LEXER_ERROR_MISSING_EXPR:
                puts("Expected an expression.");
                break;
            default:
                printf("Unrecognized error code %i.\n", err.code);
                break;
        }
    }
}
