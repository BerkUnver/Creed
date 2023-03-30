#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "print.h"

Location location_expand(Location begin, Location end) {
    return (Location) {
        .line_start = begin.line_start,
        .char_start = begin.char_start,
        .line_end = end.line_end,
        .char_end = end.char_end
    };
}

void location_print(Location location) {
    printf("(%i, %i) -> (%i, %i)", location.line_start + 1, location.char_start + 1, location.line_end + 1, location.char_end + 1);
}

char *string_operators[] = {
    "&&", "||", "&", "|", "^", "==", "!=", "<", ">", "<=", ">=", "<<", ">>", "+", "-", "*", "/", "%"
};

int operator_precedences[] = {
    0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5
};

char *string_keywords[] = {
    "if", "elif", "else", "as"
};

char *string_assigns[] = {
    "=", "&&=" , "||=", "&=", "|=", "^=", "<<=", ">>=", "+=", "-=", "*=", "/=", "%="
};

bool char_is_single_char_token_type(int c) { 
    return c == TOKEN_EOF
        || c == TOKEN_PAREN_OPEN
        || c == TOKEN_PAREN_CLOSE
        || c == TOKEN_CURLY_BRACE_OPEN
        || c == TOKEN_CURLY_BRACE_CLOSE
        || c == TOKEN_BRACKET_OPEN
        || c == TOKEN_BRACKET_CLOSE
        || c == TOKEN_BITWISE_NOT
        || c == TOKEN_QUESTION_MARK
        || c == TOKEN_COLON
        || c == TOKEN_COMMA
        || c == TOKEN_SEMICOLON
        || c == TOKEN_DOT;
}

bool char_is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

void literal_print(Literal *literal) {
    switch (literal->type) {
        case LITERAL_CHAR:
            putchar('\'');
            print_literal_char(literal->data.l_char);
            putchar('\'');
            break;

        case LITERAL_STRING:
            putchar('"');
            int idx = 0;
            char *string = string_cache_get(literal->data.l_string);
            while (string[idx] != '\0') {
                print_literal_char(string[idx]);
                idx++;
            }
            putchar('"');
            break;

        case LITERAL_INT:
            printf("%i", literal->data.l_int);
            break;

        case LITERAL_DOUBLE:
            // todo: make so this prints out only as many chars as the token is in length.
            printf("%lf", literal->data.l_double);
            break;

        default:
            printf("[Unrecognized token typen %i]", literal->type);
            break;
    }

}

void token_print(Token *token) {
    if (token->type == TOKEN_LITERAL)
        literal_print(&token->data.literal);
    else if (token->type == TOKEN_ID)
        print(string_cache_get(token->data.id));
    else if (token->type == TOKEN_EOF) // TOKEN_EOF is a single char token type so return here so it does not try to print it out.
        print("EOF");
    else if (token->type == TOKEN_UNARY_LOGICAL_NOT)
        putchar('!');
    else if (char_is_single_char_token_type(token->type))
        putchar(token->type);
    else if (TOKEN_OP_MIN <= token->type && token->type <= TOKEN_OP_MAX)
        print(string_operators[token->type - TOKEN_OP_MIN]);
    else if (TOKEN_KEYWORD_MIN <= token->type && token->type <= TOKEN_KEYWORD_MAX)
        print(string_keywords[token->type - TOKEN_KEYWORD_MIN]);
    else if (TOKEN_ASSIGN_MIN <= token->type && token->type <= TOKEN_ASSIGN_MAX)
        print(string_assigns[token->type - TOKEN_ASSIGN_MIN]);
    else if (TOKEN_ERROR_MIN <= token->type && token->type <= TOKEN_ERROR_MAX) {
        print("[Error ");
        location_print(token->location);
        print(". ");
        
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
    } else { // not printing EOF for now.
        printf("[Unrecognized token with type id %i]", token->type);
    }
}
