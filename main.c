#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "string_cache.h"
#include "symbol_table.h"

int main(int argc, char **argv) {
    string_cache_init();
    
    if (argc >= 2) {
        Lexer lexer = lexer_new(argv[1]);
        while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
            Declaration declaration = declaration_parse(&lexer);
            declaration_print(&declaration);
            declaration_free(&declaration);
            putchar('\n');
        }
        lexer_free(&lexer);
    } else {

        { // test lexer getting tokens
            Lexer lexer = lexer_new("test_lexer.txt");
            while (true) {
                Token token = lexer_token_get(&lexer);
                token_print(&token);
                print("\t\t\t[");
                location_print(token.location);
                printf(". type %i]\n", token.type);
                if (token.type == TOKEN_EOF || (TOKEN_ERROR_MIN <= token.type && token.type <= TOKEN_ERROR_MAX)) break;
            }
            lexer_free(&lexer);
        }

        putchar('\n');
       
        { // test parsing expressions.
            Lexer lexer = lexer_new("test_expr.txt");
            Expr expr = expr_parse(&lexer); 
            lexer_free(&lexer);
            expr_print(&expr);
            expr_free(&expr);
        }

        putchar('\n');

        { // test parsing scopes
            Lexer lexer = lexer_new("test_scope.txt");
            Scope scope = scope_parse(&lexer);
            lexer_free(&lexer);
            scope_print(&scope, 0);
            scope_free(&scope);
        }

        putchar('\n');

        { // test parsing declarations
            SymbolTable table = symbol_table_from_file("test_declaration.creed");
            symbol_table_print(&table);
            symbol_table_free_head(&table);
        }

        putchar('\n');

        { // test typechecking
            SymbolTable table = symbol_table_from_file("test_symbol_table.creed");
            symbol_table_check_functions(&table); 
            symbol_table_free_head(&table);
        }
    }

    string_cache_free();
    return EXIT_SUCCESS;
}
