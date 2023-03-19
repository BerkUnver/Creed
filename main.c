#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "token.h"
#include "parser.h"

#define PATH_TEST_LEXER "test_lexer.txt"
#define PATH_TEST_PARSER "test_parser.txt"

void test_lexer(void) {
    Lexer lexer;
    bool success = lexer_new(PATH_TEST_LEXER, &lexer);
    if (!success) {
        printf("Failed to open the file at "PATH_TEST_LEXER".");
        return;
    }

    while (true) {
        Token token = lexer_token_get(&lexer);
        token_print(&token);
        printf("\t\t\t[%i, %i -> %i, %i. type %i]\n", token.location.line_start + 1, token.location.char_start + 1, token.location.line_end + 1, token.location.char_end + 1, token.type);
        token_free(&token);
        if (token.type == TOKEN_EOF || (TOKEN_ERROR_MIN <= token.type && token.type <= TOKEN_ERROR_MAX)) break;
    }
    putchar('\n');
    
    lexer_free(&lexer);
}

void test_parser(void) {
    Lexer lexer;
    if (!lexer_new(PATH_TEST_PARSER, &lexer)) {
        printf("Failed to open the file at "PATH_TEST_PARSER".\n");
        return;
    }

    Expr expr = expr_parse(&lexer); 
    lexer_free(&lexer);
    expr_print(&expr);
    expr_free(&expr);
    return;
}

int main(void) {
    test_lexer();
    putchar('\n');
    test_parser();
    putchar('\n');
    return EXIT_SUCCESS;
}
