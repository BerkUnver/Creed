#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "token.h"
#include "parser.h"

#define PATH_TEST_LEXER "test_lexer.txt"
#define PATH_TEST_PARSER "test_parser.txt"

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Please put the file name as the argument to this application.");
        return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "-test_lexer"))
    {

        Lexer lexer;
        bool success = lexer_new(PATH_TEST_LEXER, &lexer);
        if (!success) {
            printf("Failed to open the file at "PATH_TEST_LEXER".");
            return EXIT_FAILURE;
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
        
        for (int i = 0; i < lexer.error_count; i++) {
            LexerError error = lexer.errors[i];
            printf("Error! Line: %i, char: %i, length: %i, code: %i\n", error.line_idx + 1, error.char_idx + 1, error.len, error.code);
        }

        lexer_free(&lexer);
        return EXIT_SUCCESS;
    
    } else if (!strcmp(argv[1], "-test_parser")) {
        Lexer lexer;
        if (!lexer_new(PATH_TEST_PARSER, &lexer)) {
            printf("Failed to open the file at "PATH_TEST_PARSER".");
            return EXIT_FAILURE;
        }

        AstExpr expr;
        if (!parse_expr(&lexer, &expr)) {
            lexer_free(&lexer);
            printf("Lexer test failed.");
            return EXIT_FAILURE;
        }
        
        lexer_free(&lexer);
        expr_free(&expr);
        return EXIT_SUCCESS;
    }
}
