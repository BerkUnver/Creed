#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "string_cache.h"
#include "symbol_table.h"
#include "handlers.h"

int main(int argc, char **argv) {
    string_cache_init();
    
    if (argc >= 2) {
        SourceFile file = source_file_parse(string_cache_insert_static(argv[1]));
        source_file_print(&file);
        typecheck(&file);
        handle_driver(&file);
        source_file_free(&file);
    } else {

        { // test lexer getting tokens
            Lexer lexer = lexer_new(string_cache_insert_static("test/lexer.txt"));
            while (true) {
                Token token = lexer_token_get(&lexer);
                token_print(&token);
                printf("\t\t\t[%i type %i]\n", token.location.idx_line + 1, token.type);
                if (token.type == TOKEN_NULL || (TOKEN_ERROR_MIN <= token.type && token.type <= TOKEN_ERROR_MAX)) break;
            }
        }

        putchar('\n');
       
        { // test parsing expressions.
            Lexer lexer = lexer_new(string_cache_insert_static("test/expr.txt"));
            Expr expr = expr_parse(&lexer); 
            expr_print(&expr, 0);
            expr_free(&expr);
        }

        putchar('\n');

        { // test parsing scopes
            Lexer lexer = lexer_new(string_cache_insert_static("test/scope.txt"));
            Scope scope = scope_parse(&lexer);
            scope_print(&scope, 0);
            putchar('\n');
            scope_free(&scope);
        }

        putchar('\n');
        
        { // test parsing a file
            SourceFile file = source_file_parse(string_cache_insert_static("test/declaration.creed"));
            source_file_print(&file);
            typecheck(&file);
            source_file_free(&file);
        }
    }

    string_cache_free();
    return EXIT_SUCCESS;
}
