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
        
        { // test parsing a file of top-level declarations.
            SourceFile source = source_file_parse("test_declaration.creed");
            source_file_print(&source);
            source_file_free(&source);
        }

        putchar('\n');
        { // test parsing a function into a symbol table.
            SourceFile source = source_file_parse("test_symbol_table.creed");
            for (int i = 0; i < source.declaration_count; i++) {
                symbol_table_check_declaration(source.declarations + i);
            }
            source_file_free(&source);
        }
    }

    string_cache_free();
    return EXIT_SUCCESS;
}
