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
    };
    return true;
}

void lexer_free(Lexer *lexer) {
    fclose(lexer->file);
    if (lexer->peek_cached) token_free(&lexer->peek);
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

// returns the literal if it is found; otherwise returns a negative number.
// If it finds a valid literal it will consume it, otherwise not.
// Currently strings and chars have the same escape sequences.
// In C, this is not true, as single quote is allowed in strings and double quote is allowed in chars.
static int lexer_literal_char_get(Lexer *lexer) {
    while (true) {
        
        int c = fpeek(lexer->file);
        if (c == '\\') { // if it is an escape sequence
            lexer_char_get(lexer);
            switch (lexer_char_get(lexer)) { // Kind of similar to how it is printed, consider consolidating.
                case '\\': return '\\';
                case 'n': return '\n';
                case 't': return '\t';
                case '0': return '\0';
                case '\'': return '\'';
                case '\"': return '\"';
                case 'r': return '\r';
                default: return -1;
            }
        } else if (' ' <= c && c <= '~' && c != '\'' && c != '\"') { // normal character
            lexer_char_get(lexer);
            return c;
        } else { // illegal char (could be ' or ")
            return -1;
        }
    }
}

static double lexer_decimal_get(Lexer *lexer) {
    double val = 0;
    double digit = 0.1;
    while (true) {
        int peek = fpeek(lexer->file);
        if (peek < '0' || '9' < peek) return val; 
        lexer_char_get(lexer);
        val += (digit * (peek - '0'));
        digit /= 10;
    }
}

static Token lexer_token_get_skip_cache(Lexer *lexer) {
    // consume whitespace
    while (char_is_whitespace(fpeek(lexer->file))) lexer_char_get(lexer);

    Token token;
    token.location = (Location) {
        .line_start = lexer->line_idx,
        .line_end = lexer->line_idx,
        .char_start = lexer->char_idx,
    };

    int peek = fpeek(lexer->file); 
    if (char_is_single_char_token_type(peek)) {
        token.type = lexer_char_get(lexer); // chars corresponds 1:1 to unique single-char token types.
    
    } else if (peek == '0') { // get decimals that start with 0.
        lexer_char_get(lexer);
        int int_peek_2 = fpeek(lexer->file);
        if ('0' <= int_peek_2 && int_peek_2 <= '9') { // in error state
            token.type = TOKEN_ERROR_LITERAL_NUMBER_LEADING_ZERO;
        } else if (int_peek_2 == '.') {
            lexer_char_get(lexer);
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_DOUBLE;
            token.data.literal.data.double_float = lexer_decimal_get(lexer);
        } else {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_INT;
            token.data.literal.data.integer = 0;
        }
    
    } else if ('1' <= peek && peek <= '9') {
        int literal_int = lexer_char_get(lexer) - '0';
        while (true) {
            int digit = fpeek(lexer->file);
            if (digit < '0' || '9' < digit) break;
            lexer_char_get(lexer);
            literal_int *= 10;
            literal_int += (digit - '0'); // todo: bounds-checking.
        }

        if (fpeek(lexer->file) == '.') {
            lexer_char_get(lexer);
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_DOUBLE;
            token.data.literal.data.double_float = ((double) literal_int) + lexer_decimal_get(lexer);
        
        } else {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_INT;
            token.data.literal.data.integer = literal_int;
        }   

    } else if (peek == DELIMITER_LITERAL_CHAR) {
        
        lexer_char_get(lexer);
        int literal_char = lexer_literal_char_get(lexer);
        
        
        if (literal_char < 0) { // error condition
            token.type = TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER;
        } else if (fpeek(lexer->file) != DELIMITER_LITERAL_CHAR) {
            token.type = TOKEN_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING;
        } else {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_CHAR;
            token.data.literal.data.character = literal_char;
            lexer_char_get(lexer);
        }
    } else if (peek == DELIMITER_LITERAL_STRING) {
        lexer_char_get(lexer);
        StringBuilder builder = string_builder_new();
        
        int c;
        while ((c = lexer_literal_char_get(lexer)) >= 0) {
            string_builder_add_char(&builder, c);
        }

        char *string = string_builder_free(&builder); 
        
        if (fpeek(lexer->file) == DELIMITER_LITERAL_STRING) {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_STRING;
            token.data.literal.data.string = string;
            lexer_char_get(lexer);
        } else {
            free(string);
            token.type = TOKEN_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING;
        }

    } else if (char_is_operator(peek)) {

        char operator[OPERATOR_MAX_LENGTH + 1];
   
        int operator_length = 0;
        while (char_is_operator(fpeek(lexer->file))) {
            char c = lexer_char_get(lexer);
            if (operator_length < OPERATOR_MAX_LENGTH) operator[operator_length] = c;
            operator_length++;
        }

        if (operator_length > OPERATOR_MAX_LENGTH) {
            token.type = TOKEN_ERROR_OPERATOR_TOO_LONG;
        } else {
            operator[operator_length] = '\0';
            bool is_operator = false;
            for (int i = 0; i < sizeof(string_operators) / sizeof(*string_operators); i++) {
                if (!strcmp(operator, string_operators[i])) {
                    token.type = TOKEN_OP_MIN + i;
                    is_operator = true;
                    break;
                }
            }

            if (!is_operator) {
                token.type = TOKEN_ERROR_OPERATOR_UNKNOWN;
            }
        }

    } else if (char_is_identifier(peek)) {
        StringBuilder builder = string_builder_new();
        string_builder_add_char(&builder, lexer_char_get(lexer));
        while (char_is_identifier(fpeek(lexer->file))) {
            string_builder_add_char(&builder, lexer_char_get(lexer));
        }
        
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
        token.type = TOKEN_ERROR_CHARACTER_UNKNOWN;
    }
    
    token.location.char_end = lexer->char_idx;
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
