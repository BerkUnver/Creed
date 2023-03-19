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
    free(lexer->errors);
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

static bool lexer_char_get_if(Lexer *lexer, char c) {
    if (fpeek(lexer->file) == c) {
        lexer_char_get(lexer);
        return true;
    }
    return false;
}

// returns the literal if it is found; otherwise returns a negative number.
// If it finds a valid literal it will consume it, otherwise not.
// Currently strings and chars have the same escape sequences.
// In C, this is not true, as single quote is allowed in strings and double quote is allowed in chars.
static int lexer_literal_char_get(Lexer *lexer) {
    while (true) {
        int c = fpeek(lexer->file);
        if (c == '\\') { // if it is an escape sequence 
            Location location = {
                .line_start = lexer->line_idx,
                .char_start = lexer->char_idx
            };
            lexer_char_get(lexer);
            switch (lexer_char_get(lexer)) { // Kind of similar to how it is printed, consider consolidating.
                case '\\': return '\\';
                case 'n': return '\n';
                case 't': return '\t';
                case '0': return '\0';
                case '\'': return '\'';
                case '\"': return '\"';
                case 'r': return '\r';
                default:
                    location.line_end = lexer->line_idx;
                    location.char_end = lexer->char_idx;
                    lexer_error_push(lexer, location, LEXER_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE);
                    break;
            }
        } else if (' ' <= c && c <= '~' && c != '\'' && c != '\"') { // normal character
            lexer_char_get(lexer);
            return c;
        } else { // illegal char (could be ' or ")
            return -1;
        }
    }
}

void lexer_error_push(Lexer *lexer, Location location, LexerError error) {
    lexer->error_count++;
    lexer->errors = realloc(lexer->errors, sizeof(LexerError) * lexer->error_count);
    lexer->errors[lexer->error_count - 1].location = location;
    lexer->errors[lexer->error_count - 1].error = error;
}

static void lexer_error_push_token(Lexer *lexer, Token *token, LexerError error) {
    Location location = token->location;
    location.line_end = lexer->line_idx;
    location.char_end = lexer->char_idx;
    lexer_error_push(lexer, location, error);
}

void lexer_error_print(Lexer *lexer) {
    for (int i = 0; i < lexer->error_count; i++) {
        Location location = lexer->errors[i].location;
        if (location.line_start == location.line_end) {
            int len = location.char_end - location.char_start;
            printf("[Error at line %i, char %i, len %i. ", location.line_start + 1, location.char_start + 1, len);
        } else {
            printf("[Error from line %i, char %i to line %i, char %i. ", location.line_start + 1, location.char_start + 1, location.line_end + 1, location.char_end + 1);
        }

        switch (lexer->errors[i].error) {
            case LEXER_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE:
                print("This is not a valid character escape sequence.");
                break;
            case LEXER_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER:
                print("This is not a valid character literal.");
                break;
            case LEXER_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING:
                printf("A character literal must end with %c.", DELIMITER_LITERAL_CHAR);
                break;
            case LEXER_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING:
                printf("A string literal must end with %c.", DELIMITER_LITERAL_STRING);
                break;
            case LEXER_ERROR_OP_TOO_LONG:
                printf("No existing operator exceeds the length %i", OPERATOR_MAX_LENGTH);
                break;
            case LEXER_ERROR_OP_UNKNOWN:
                print("This operator is unknown.");
                break;
            case LEXER_ERROR_CHAR_UNKNOWN:
                print("This character is not allowed in source files.");
                break;
        }

        print("]\n");
    }
}

static Token lexer_token_get_skip_cache(Lexer *lexer) {
    // consume whitespace
    while (char_is_whitespace(fpeek(lexer->file))) lexer_char_get(lexer);

    Token token;
    token.location = (Location) {
        .line_start = lexer->line_idx,
        .line_end = lexer->line_idx, // no multiline tokens right now, could change in the future (multiline string literals?)
        .char_start = lexer->char_idx,
    };

    int peek = fpeek(lexer->file); 
    if (char_is_single_char_token_type(peek)) {
        token.type = lexer_char_get(lexer); // chars corresponds 1:1 to unique single-char token types.
   
    } else if ('0' <= peek && peek <= '9') {
        token.type = TOKEN_LITERAL;

        int literal_int = 0;
        while (true) {
            int digit = fpeek(lexer->file);
            if (digit < '0' || '9' < digit) break;
            lexer_char_get(lexer);
            literal_int *= 10;
            literal_int += (digit - '0');
        }

        if (fpeek(lexer->file) == '.') {
            lexer_char_get(lexer);
            token.data.literal.type = LITERAL_DOUBLE;
            token.data.literal.data.double_float = (double) literal_int;
            double digit = 0.1;
            while (true) {
                int digit_char = fpeek(lexer->file);
                if (digit_char < '0' || '9' < digit_char) break;
                lexer_char_get(lexer);
                token.data.literal.data.double_float += (digit * (digit_char - '0'));
                digit /= 10;
            }
        } else {
            token.type = LITERAL_INT;
            token.data.literal.data.integer = literal_int;
        }
    } else if (peek == DELIMITER_LITERAL_CHAR) {    
        lexer_char_get(lexer);

        int literal_char = lexer_literal_char_get(lexer);
       
        token.type = TOKEN_LITERAL;
        token.data.literal.type = LITERAL_CHAR;
        token.data.literal.data.character = literal_char;

        if (fpeek(lexer->file) == DELIMITER_LITERAL_CHAR)
            lexer_char_get(lexer);
        else 
            lexer_error_push_token(lexer, &token, LEXER_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING);

    } else if (peek == DELIMITER_LITERAL_STRING) {
        lexer_char_get(lexer);
        StringBuilder builder = string_builder_new();
        
        int c;
        while ((c = lexer_literal_char_get(lexer)) >= 0) {
            string_builder_add_char(&builder, c);
        }
        
        token.type = TOKEN_LITERAL;
        token.data.literal.type = LITERAL_STRING;
        token.data.literal.data.string = string_builder_free(&builder); 
        
        if (fpeek(lexer->file) == DELIMITER_LITERAL_STRING)
            lexer_char_get(lexer);
        else
            lexer_error_push_token(lexer, &token, LEXER_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING);

    } else if (char_is_operator(peek)) {
        lexer_char_get(lexer);
        switch (peek) {
            case '=':
                token.type = (lexer_char_get_if(lexer, '=')) ? TOKEN_OP_EQ : TOKEN_ASSIGN;
                break;
            case '<':
                if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_OP_LE;
                else if (lexer_char_get_if(lexer, '<'))
                    token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_SHIFT_LEFT : TOKEN_OP_SHIFT_LEFT;
                else
                    token.type = TOKEN_OP_LT;
                break;

            case '>':
                if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_OP_GE;
                else if (lexer_char_get_if(lexer, '>'))
                    token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_SHIFT_RIGHT : TOKE
                if (fpeek(lexer->file) == '=') {
                    lexer_char_get(lexer);
                    token.type = TOKEN_OP_LE;
                } else token.type = TOKEN_OP_LT;
                break;
            
            case '!':
                token.type = (lexer_char_get_if(lexer, '=')) ? TOKEN_NE : TOKEN_UNARY_NOT;
                break;

            case '+'
                if (lexer_char_get_if(lexer, '+'))
                    token.type = TOKEN_UNARY_INCREMENT;
                else if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_ASSIGN_PLUS;
                else
                    token.type = TOKEN_OP_PLUS;
                break;
            
            case '-'
                if (lexer_char_get_if(lexer, '-'))
                    token.type = TOKEN_UNARY_DEINCREMENT;
                else if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_ASSIGN_MINUS;
                else
                    token.type = TOKEN_OP_MINUS;
                break;
            
            case '/': // todo: add multiline comments
                if (lexer_char_get_if(lexer, '/')) { // is a comment, skip
                    while (fpeek(lexer->file) != '\n') lexer_char_get(lexer);
                    return lexer_token_get_skip_cache(lexer);
                } else if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_ASSIGN_DIVIDE;
                else 
                    token.type = TOKEN_OP_DIVIDE;
                break;

            case '*':
                token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_MULTIPLY : TOKEN_OP_MULTIPLY;
                break;

            case '%':
                token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_MODULO : TOKEN_OP_MODULO;
                break;

            case '&':
                if (lexer_char_get_if(lexer, '&'))
                    token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_LOGICAL_AND : TOKEN_OP_LOGICAL_AND;
                else if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_ASSIGN_BITWISE_AND;
                else
                    token.type = TOKEN_OP_BITWISE_AND;
                break;

            case '|':
                if (lexer_char_get_if(lexer, '|'))
                    token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_LOGICAL_OR : TOKEN_OP_LOGICAL_OR;
                else if (lexer_char_get_if(lexer, '='))
                    token.type = TOKEN_ASSIGN_BITWISE_OR;
                else
                    token.type = TOKEN_OP_BITWISE_OR;
                break;

            case '^':
                token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_BITWISE_XOR : TOKEN_OP_BITWISE_XOR;
                break;

            case '~':
                token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_BITWISE_NOT : TOKEN_OP_BITWISE_NOT;
                break;
        }
        char operator[OPERATOR_MAX_LENGTH + 1];
   
        int operator_length = 0;
        while (char_is_operator(fpeek(lexer->file))) {
            char c = lexer_char_get(lexer);
            if (operator_length < OPERATOR_MAX_LENGTH) operator[operator_length] = c;
            operator_length++;
        }

        if (operator_length > OPERATOR_MAX_LENGTH) {
            lexer_error_push_token(lexer, &token, LEXER_ERROR_OP_TOO_LONG);
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
                lexer_error_push_token(lexer, &token, LEXER_ERROR_OP_UNKNOWN);
                token.type = TOKEN_OP_DUMMY;
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
        lexer_error_push_token(lexer, &token, LEXER_ERROR_CHAR_UNKNOWN);
        return lexer_token_get_skip_cache(lexer);
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
