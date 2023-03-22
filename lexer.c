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

static Token lexer_token_get_skip_cache(Lexer *lexer) {
    // consume whitespace
    while (char_is_whitespace(fpeek(lexer->file))) lexer_char_get(lexer);

    Token token;
    token.location = (Location) {
        .line_start = lexer->line_idx,
        .line_end = lexer->line_idx,
        .char_start = lexer->char_idx,
    };

    int char_first = lexer_char_get(lexer);
    
    switch (char_first) {
    case DELIMITER_LITERAL_CHAR: { 
        int literal_char = lexer_literal_char_get(lexer);
        if (literal_char < 0) { // error condition
            token.type = TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER;
        } else if (fpeek(lexer->file) != DELIMITER_LITERAL_CHAR) {
            token.type = TOKEN_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING;
        } else {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_CHAR;
            token.data.literal.data.l_char = literal_char;
            lexer_char_get(lexer);
        }
    } break;

    case DELIMITER_LITERAL_STRING: {
        StringBuilder builder = string_builder_new();
        
        int c;
        while ((c = lexer_literal_char_get(lexer)) >= 0) {
            string_builder_add_char(&builder, c);
        }

        char *string = string_builder_free(&builder); 
        
        if (fpeek(lexer->file) == DELIMITER_LITERAL_STRING) {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_STRING;
            token.data.literal.data.l_string = string;
            lexer_char_get(lexer);
        } else {
            free(string);
            token.type = TOKEN_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING;
        }
    } break;

    case '=':
        token.type = lexer_char_get_if(lexer, '=') ? TOKEN_OP_EQ : TOKEN_ASSIGN;
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
            token.type = lexer_char_get_if(lexer, '=') ? TOKEN_ASSIGN_SHIFT_RIGHT : TOKEN_OP_SHIFT_RIGHT;
        else 
            token.type = TOKEN_OP_GT;
        break;
    
    case '!':
        token.type = (lexer_char_get_if(lexer, '=')) ? TOKEN_OP_NE : TOKEN_UNARY_LOGICAL_NOT;
        break;

    case '+':
        if (lexer_char_get_if(lexer, '+'))
            token.type = TOKEN_INCREMENT;
        else if (lexer_char_get_if(lexer, '='))
            token.type = TOKEN_ASSIGN_PLUS;
        else
            token.type = TOKEN_OP_PLUS;
        break;
    
    case '-':
        if (lexer_char_get_if(lexer, '-'))
            token.type = TOKEN_DEINCREMENT;
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

    default:
        if (char_is_single_char_token_type(char_first)) 
            token.type = char_first;
        else if ('0' <= char_first && char_first <= '9') {
            token.type = TOKEN_LITERAL;
            int literal_int = char_first - '0';
            
            while (true) {
                int digit = fpeek(lexer->file);
                if (digit < '0' || '9' < digit) break;
                lexer_char_get(lexer);
                literal_int *= 10;
                literal_int += (digit - '0');
            }
            
            if (lexer_char_get_if(lexer, '.')) {
                token.data.literal.type = LITERAL_DOUBLE;
                token.data.literal.data.l_double = (double) literal_int;
                double digit = 0.1;
                while (true) {
                    int digit_char = fpeek(lexer->file);
                    if (digit_char < '0' || '9' < digit_char) break;
                    lexer_char_get(lexer);
                    token.data.literal.data.l_double += (digit * (digit_char - '0'));
                    digit /= 10;
                }
            } else {
                token.data.literal.type = LITERAL_INT;
                token.data.literal.data.l_int = literal_int;
            }
        } else if (('a' <= char_first && char_first <= 'z') || ('A' <= char_first && char_first <= 'Z') || char_first == '_') {
            StringBuilder builder = string_builder_new();
            string_builder_add_char(&builder, char_first);

            while (true) {
                char c = fpeek(lexer->file);
                if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_') || ('0' <= c && c <= '9'))
                    string_builder_add_char(&builder, lexer_char_get(lexer));
                else break;
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
            token.type = TOKEN_ERROR_CHARACTER_UNKNOWN;
        }
        break;
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
