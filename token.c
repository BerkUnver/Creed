#include <stdlib.h>
#include "token.h"

const TokenType single_char_tokens[] = {
    TOKEN_EOF,
    TOKEN_PAREN_OPEN,
    TOKEN_PAREN_CLOSE,
    TOKEN_CURLY_BRACE_OPEN,
    TOKEN_CURLY_BRACE_CLOSE,
    TOKEN_BRACKET_OPEN,
    TOKEN_BRACKET_CLOSE,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_DOT
};

bool char_is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool char_is_escape(char c) {
    return c == '\n' || c == '\"' || c == '\'' || c == '\n' || c == '\t' || c == '\r' || c == '\0' || c == '\\';
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

void token_print(Token *token) {
    switch (token->type) {
        case TOKEN_ERROR:
            printf("Error at line: %i, char: %i, len: %i. Message: %s", token->line_idx + 1, token->char_idx + 1, token->len, token->data.error);
            break;
        case TOKEN_IF:
            fputs(STR_IF, stdout);
            break;
        case TOKEN_ELIF:
            fputs(STR_ELIF, stdout);
            break;
        case TOKEN_ELSE:
            fputs(STR_ELSE, stdout);
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
            for (int i = 0; i < sizeof(single_char_tokens); i++) {
                if (token->type == single_char_tokens[i]) {
                    putchar(token->type);
                    return;
                }
            }
            printf("[Unrecognized token with type id %i]", token->type);
            break;
    }
}
