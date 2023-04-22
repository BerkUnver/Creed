#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "print.h"
#include "string_cache.h"
#include "symbol_table.h"

#define PATH_TEST_LEXER "test_lexer.txt"
#define PATH_TEST_EXPR "test_expr.txt"
#define PATH_TEST_SCOPE "test_scope.txt"

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
        print("\t\t\t[");
        location_print(token.location);
        printf(". type %i]\n", token.type);
        if (token.type == TOKEN_EOF || (TOKEN_ERROR_MIN <= token.type && token.type <= TOKEN_ERROR_MAX)) break;
    }
    putchar('\n');
    
    lexer_free(&lexer);
}

void test_expr(void) {
    Lexer lexer;
    if (!lexer_new(PATH_TEST_EXPR, &lexer)) {
        printf("Failed to open the file at "PATH_TEST_EXPR".\n");
        return;
    }

    Expr expr = expr_parse(&lexer); 
    lexer_free(&lexer);
    expr_print(&expr);
    expr_free(&expr);
    return;
}

void test_scope(void) {
    Lexer lexer;
    if (!lexer_new(PATH_TEST_SCOPE, &lexer)) {
        printf("Failed to open the file at "PATH_TEST_SCOPE".\n");
        return;
    }

    Scope scope = scope_parse(&lexer);
    lexer_free(&lexer);
    scope_print(&scope, 0);
    scope_free(&scope);
    return;
}

int main(int argc, char **argv) {
    string_cache_init();
    
    if (argc >= 2) {
        Lexer lexer;
        if (!lexer_new(argv[1], &lexer)) {
            printf("Could not read the file at \"%s\".", argv[1]);
        } else { 
            while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
                Declaration declaration = declaration_parse(&lexer);
                declaration_print(&declaration);
                declaration_free(&declaration);
                putchar('\n');
            }
            lexer_free(&lexer);
        }
    } else {
        test_lexer();
        putchar('\n');
        test_expr();
        putchar('\n');
        test_scope();
        putchar('\n');
        SourceFile source_declaration = source_file_parse("test_declaration.creed");
        source_file_print(&source_declaration);
        source_file_free(&source_declaration);

        SourceFile source_symbol_table = source_file_parse("test_symbol_table.creed");
        for (int i = 0; i < source_symbol_table.declaration_count; i++) {
            symbol_table_check_declaration(source_symbol_table.declarations + i);
        }
        source_file_free(&source_symbol_table);
    }

    string_cache_free();
    return EXIT_SUCCESS;
}
