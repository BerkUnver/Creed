#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "lexer.h"
#include "prelude.h"
#include "string_builder.h"
#include "token.h"


Lexer lexer_new(StringId path) {
    
    struct stat st;
    const char *path_string = string_cache_get(path);
    FILE *file = fopen(path_string, "r");
    if (stat(path_string, &st) < 0 || !file) {
        printf("Failed to read file %s.", path_string);
        exit(EXIT_FAILURE);
    }

    char *str = malloc(st.st_size + sizeof(char));
    size_t size = fread(str, 1, st.st_size, file);
    fclose(file);
    str[size / sizeof(char)] = '\0';

    StringId file_content = string_cache_insert(str); 

    return (Lexer) {
        .file_name = path,
        .file_content = file_content,
        .file_content_ptr = string_cache_get(file_content),
        .idx_line = 0,
        .idx_char = 0,
        .peek_idx = 0,
        .peek_count = 0
    };
}

static int lexer_char_peek(Lexer *lexer) {
   return lexer->file_content_ptr[lexer->idx_char];
}

static int lexer_char_get(Lexer *lexer) {
    int c = lexer->file_content_ptr[lexer->idx_char];
    if (c == '\n') lexer->idx_line++;
    if (c != '\0') lexer->idx_char++;
    return c;
}

static bool lexer_char_get_if(Lexer *lexer, char c) {
    if (lexer_char_peek(lexer) == c) {
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
        
        int c = lexer_char_peek(lexer);
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
    while (char_is_whitespace(lexer_char_peek(lexer))) lexer_char_get(lexer);

    Token token;
    token.location = (Location) {
        .file_name = lexer->file_name,
        .file_content = lexer->file_content,
        .idx_line = lexer->idx_line,
        .idx_start = lexer->idx_char
    };

    int char_first = lexer_char_get(lexer);
    
    switch (char_first) {
    case DELIMITER_LITERAL_CHAR: { 
        int literal_char = lexer_literal_char_get(lexer);
        if (literal_char < 0) { // error condition
            token.type = TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER;
        } else if (lexer_char_peek(lexer) != DELIMITER_LITERAL_CHAR) {
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
        StringId literal_string = string_cache_insert(string_builder_free(&builder));

        if (lexer_char_peek(lexer) == DELIMITER_LITERAL_STRING) {
            token.type = TOKEN_LITERAL;
            token.data.literal.type = LITERAL_STRING;
            token.data.literal.data.l_string = literal_string;
            lexer_char_get(lexer);
        } else {
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
        else if (lexer_char_get_if(lexer, '>'))
            token.type = TOKEN_LAMBDA;
        else
            token.type = TOKEN_OP_MINUS;
        break;
    
    case '/': // todo: add multiline comments
        if (lexer_char_get_if(lexer, '/')) { // is a comment, skip
            while (lexer_char_peek(lexer) != '\n') lexer_char_get(lexer);
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
            unsigned long long literal_uint64 = char_first - '0';
            double literal_float64;
            
            while (true) {
                int digit = lexer_char_peek(lexer);
                if (digit < '0' || '9' < digit) break;
                lexer_char_get(lexer);
                literal_uint64 *= 10;
                literal_uint64 += (digit - '0');
            }
           
            bool is_float = false;
            literal_float64 = (double) literal_uint64; // need to set in case the format specifier is float
            if (lexer_char_get_if(lexer, '.')) {
                is_float = true;
                double digit = 0.1;
                while (true) {
                    int digit_char = lexer_char_peek(lexer);
                    if (digit_char < '0' || '9' < digit_char) break;
                    lexer_char_get(lexer);
                    literal_float64 += (digit * (digit_char - '0'));
                    digit /= 10;
                }
            }
            
            // Check for number literal type specifier 
            char c = lexer_char_peek(lexer);
            if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_')) {
                StringBuilder builder = string_builder_new();
                string_builder_add_char(&builder, lexer_char_get(lexer));
                
                while (true) {
                    c = lexer_char_peek(lexer);
                    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_') || ('0' <= c && c <= '9'))
                        string_builder_add_char(&builder, lexer_char_get(lexer));
                    else break;
                }
                
                char *string = string_builder_free(&builder);

                if (!strcmp(string, "f")) {
                    token.data.literal.type = LITERAL_FLOAT;
                    token.data.literal.data.l_float = (float) literal_float64;
                } else if (!strcmp(string, "f64")) {
                    token.data.literal.type = LITERAL_FLOAT64;
                    token.data.literal.data.l_float64 = literal_float64;
                } else if (!is_float) {
                    if (!strcmp(string, "i8")) {
                        token.data.literal.type = LITERAL_INT8;
                        token.data.literal.data.l_int8 = (char) literal_uint64;
                    } else if (!strcmp(string, "i16")) {
                        token.data.literal.type = LITERAL_INT16;
                        token.data.literal.data.l_int16 = (short) literal_uint64;
                    } else if (!strcmp(string, "i")) {
                        token.data.literal.type = LITERAL_INT;
                        token.data.literal.data.l_int = (int) literal_uint64;
                    } else if (!strcmp(string, "i64")) {
                        token.data.literal.type = LITERAL_INT64;
                        token.data.literal.data.l_int64 = (long long) literal_uint64;
                    } else if (!strcmp(string, "u8")) {
                        token.data.literal.type = LITERAL_UINT8;
                        token.data.literal.data.l_uint64 = (unsigned char) literal_uint64;
                    } else if (!strcmp(string, "u16")) {
                        token.data.literal.type = LITERAL_UINT16;
                        token.data.literal.data.l_uint64 = (unsigned short) literal_uint64;
                    } else if (!strcmp(string, "u")) {
                        token.data.literal.type = LITERAL_UINT;
                        token.data.literal.data.l_uint64 = (unsigned int) literal_uint64;
                    } else if (!strcmp(string, "u64")) {
                        token.data.literal.type = LITERAL_UINT64;
                        token.data.literal.data.l_uint64 = literal_uint64;
                    } else {
                        token.type = TOKEN_ERROR_LITERAL_NUMBER_ILLEGAL_TYPE_SPEC;
                    }
                } else {
                    token.type = TOKEN_ERROR_LITERAL_NUMBER_ILLEGAL_TYPE_SPEC;
                }

                free(string);
            } else if (is_float) {
                token.data.literal.type = LITERAL_FLOAT;
                token.data.literal.data.l_float = (float) literal_float64;
            } else {
                token.data.literal.type = LITERAL_INT;
                token.data.literal.data.l_int = (int) literal_uint64;
            }

        } else if (('a' <= char_first && char_first <= 'z') || ('A' <= char_first && char_first <= 'Z') || char_first == '_') {
            StringBuilder builder = string_builder_new();
            string_builder_add_char(&builder, char_first);

            while (true) {
                char c = lexer_char_peek(lexer);
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
                token.data.id = string_cache_insert(string);
            }
        } else {
            token.type = TOKEN_ERROR_CHARACTER_UNKNOWN;
        }
        break;
    }

    token.location.idx_end = lexer->idx_char;
    return token;
}

Token lexer_token_peek(Lexer *lexer) {
    if (lexer->peek_count < 1) {
        lexer->peeks[lexer->peek_idx] = lexer_token_get_skip_cache(lexer); 
        lexer->peek_count = 1;
    }
    return lexer->peeks[lexer->peek_idx];
}

Token lexer_token_peek_2(Lexer *lexer) {
    for (; lexer->peek_count < 2; lexer->peek_count++) {
        lexer->peeks[(lexer->peek_idx + lexer->peek_count) % LEXER_TOKEN_PEEK_MAX] = lexer_token_get_skip_cache(lexer);
    }
    return lexer->peeks[(lexer->peek_idx + 1) % LEXER_TOKEN_PEEK_MAX];
}

Token lexer_token_peek_many(Lexer *lexer, int count) {
    assert(0 < count && count <= LEXER_TOKEN_PEEK_MAX);
    for (; lexer->peek_count < count; lexer->peek_count++) {
        lexer->peeks[(lexer->peek_idx + lexer->peek_count) % LEXER_TOKEN_PEEK_MAX] = lexer_token_get_skip_cache(lexer);
    }
    return lexer->peeks[(lexer->peek_idx + count - 1) % LEXER_TOKEN_PEEK_MAX]; 
}

Token lexer_token_get(Lexer *lexer) {
    if (lexer->peek_count > 0) {
        lexer->peek_count--;
        int idx = lexer->peek_idx;
        lexer->peek_idx = (lexer->peek_idx + 1) % LEXER_TOKEN_PEEK_MAX;
        return lexer->peeks[idx];
    }
    return lexer_token_get_skip_cache(lexer);
}
