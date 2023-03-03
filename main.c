#include <stdlib.h>
#include <stdio.h>
#include "lexer.h"
#include "token.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Please put the file name as the argument to this application.");
        return EXIT_FAILURE;
    }
    
    Lexer lexer;
    bool success = lexer_new(argv[1], &lexer);
    if (!success) {
        printf("Failed to open the file at %s.", argv[1]);
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
}
