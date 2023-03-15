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
        TokenType type = token.type;
        putchar(' ');
        token_print(&token);
        token_free(&token);
        if (type == TOKEN_EOF) break;
    }
    putchar('\n');
    
    lexer_error_print(&lexer);
    lexer_free(&lexer);
}

void test_parser(void) {
    Lexer lexer;
    if (!lexer_new(PATH_TEST_PARSER, &lexer)) {
        printf("Failed to open the file at "PATH_TEST_PARSER".\n");
        return;
    }

    Expr expr = expr_parse(&lexer);
    lexer_error_print(&lexer); 
    lexer_free(&lexer);
    expr_print(&expr);
    expr_free(&expr);
    return;
}

int main(void) {
    test_lexer();
    test_parser();
    return EXIT_SUCCESS;
}
