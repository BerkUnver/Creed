#include <stdlib.h>
#include "token.h"

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
    return c == '!' || c == '%' || c == '*' || c == '/' || c == '-' || c == '>' || c == '<' || c == '/' || c == '&' || c == '|' || c == '=';
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


void print(const char *str) {
    fputs(str, stdout);
}

void token_print(Token *token) {
    switch (token->type) {
        case TOKEN_ERROR:
            printf("Error at line: %i, char: %i, len: %i. Message: %s", token->line_idx + 1, token->char_idx + 1, token->len, token->data.error);
            break;
        case TOKEN_IF:
            print(STR_IF);
            break;
        case TOKEN_ELIF:
            print(STR_ELIF);
            break;
        case TOKEN_ELSE:
            print(STR_ELSE);
            break;
        case TOKEN_LITERAL_CHAR:
            putchar('\'');
            switch (token->data.literal_char) {
                case '\\': print("\\\\"); break;
                case '\n': print("\\n"); break;
                case '\t': print("\\t"); break;
                case '\0': print("\\0"); break;
                case '\'': print("\\'"); break;
                case '\"': print("\\\""); break;
                case '\r': print("\\r"); break;
                default: putchar(token->data.literal_char); break;
            }
            putchar('\'');
            break;
        case TOKEN_LITERAL_STRING:
            printf("\"%s\"", token->data.literal_string);
            break;
        case TOKEN_EQUALS:
            fputs(STR_EQUALS, stdout);
            break;
        case TOKEN_ASSIGN: 
            fputs(STR_ASSIGN, stdout); 
            break;
        case TOKEN_EOF: // EOF is a character. Do not print out. 
            break;
        default:
            if (char_is_single_char_token_type(token->type)) {
                putchar(token->type);
            } else {
                printf("[Unrecognized token with type id %i]", token->type);
            }
            break;
    }
}
