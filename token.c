#include <assert.h>
#include <stdlib.h>

#include "token.h"
#include "print.h"

char *string_operators[] = {
    "&&", "||", "&", "|", "^", "==", "!=", "<", ">", "<=", ">=", "<<", ">>", "+", "-", "*", "/", "%"
};

int operator_precedences[] = {
    0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5
};

char *string_keywords[] = {
    "if", "elif", "else", "as"
};

bool char_is_single_char_token_type(int c) { 
    return c == TOKEN_EOF
        || c == TOKEN_PAREN_OPEN
        || c == TOKEN_PAREN_CLOSE
        || c == TOKEN_CURLY_BRACE_OPEN
        || c == TOKEN_CURLY_BRACE_CLOSE
        || c == TOKEN_BRACKET_OPEN
        || c == TOKEN_BRACKET_CLOSE
        || c == TOKEN_COLON
        || c == TOKEN_COMMA
        || c == TOKEN_SEMICOLON
        || c == TOKEN_DOT;
}

bool char_is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool char_is_operator(char c) {
    return c == '!' || c == '%' || c == '*' || c == '/' || c == '+' || c == '-' || c == '>' || c == '<' || c == '/' || c == '&' || c == '|' || c == '=';
}

bool char_is_identifier(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

void token_free(Token *token) {
    switch (token->type) {
        case TOKEN_LITERAL_STRING: 
            free(token->data.literal_string);
            break;
        case TOKEN_ID:
            free(token->data.id);
            break;

        default: break; 
    }
}

void token_print(Token *token) {
    switch (token->type) {
        case TOKEN_LITERAL_CHAR:
            putchar('\'');
            print_literal_char(token->data.literal_char);
            putchar('\'');
            break;
        case TOKEN_LITERAL_STRING:
            putchar('"');
            int idx = 0;
            while (token->data.literal_string[idx] != '\0') {
                print_literal_char(token->data.literal_string[idx]);
                idx++;
            }
            putchar('"');
            break;
        case TOKEN_LITERAL_INT:
            printf("%i", token->data.literal_int);
            break;
        case TOKEN_LITERAL_DOUBLE:
            // todo: make so this prints out only as many chars as the token is in length.
            printf("%lf", token->data.literal_double);
            break;
        case TOKEN_ID:
            print(token->data.id);
            break;
        case TOKEN_EOF: // EOF is a character. Do not print out.
            break;
        default:
            if (char_is_single_char_token_type(token->type)) {
                putchar(token->type);
            } else if (TOKEN_OP_MIN <= token->type && token->type <= TOKEN_OP_MAX) {
                print(string_operators[token->type - TOKEN_OP_MIN]);
            } else if (TOKEN_KEYWORD_MIN <= token->type && token->type <= TOKEN_KEYWORD_MAX) {
                print(string_keywords[token->type - TOKEN_KEYWORD_MIN]);
            } else if (TOKEN_ERROR_MIN <= token->type && token->type <= TOKEN_ERROR_MAX) {    
                printf("[Error at line %i, char %i, len %i. ", token->line_idx + 1, token->char_idx + 1, token->len);
                switch (token->type) {
                    case TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_ESCAPE:
                        print("This is not a valid character escape sequence.");
                        break;
                    case TOKEN_ERROR_LITERAL_CHAR_ILLEGAL_CHARACTER:
                        print("This is not a valid character literal.");
                        break;
                    case TOKEN_ERROR_LITERAL_CHAR_CLOSING_DELIMITER_MISSING:
                        printf("A character literal must end with %c.", DELIMITER_LITERAL_CHAR);
                        break;
                    case TOKEN_ERROR_LITERAL_STRING_CLOSING_DELIMITER_MISSING:
                        printf("A string literal must end with %c.", DELIMITER_LITERAL_STRING);
                        break;
                    case TOKEN_ERROR_LITERAL_NUMBER_LEADING_ZERO:
                        print("A literal number cannot have a leading zero.");
                        break;
                    case TOKEN_ERROR_OPERATOR_TOO_LONG:
                        printf("No existing operator exceeds the length %i", OPERATOR_MAX_LENGTH);
                        break;
                    case TOKEN_ERROR_OPERATOR_UNKNOWN:
                        print("This operator is unknown.");
                        break;
                    case TOKEN_ERROR_CHARACTER_UNKNOWN:
                        print("This character is not allowed in source files.");
                        break;
                    default:
                        assert(false);
                        break;
                }
                putchar(']');
            } else {
                printf("[Unrecognized token with type id %i]", token->type);
            }
            break;
    }
}
