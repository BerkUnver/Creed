#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "string_cache.h"

int main(int argc, char **argv) {
    string_cache_init();
    
    if (argc >= 2) {
        Lexer lexer = lexer_new(argv[1]);
        while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
            Statement statement = statement_parse(&lexer);
            if (lexer_token_get(&lexer).type != TOKEN_SEMICOLON) {
                error_exit(statement.location, "Expected a semicolon after a top-level statement.");
            }
            statement_print(&statement, 0);
            statement_free(&statement);
            putchar('\n');
        }
        lexer_free(&lexer);
    } else {

        { // test lexer getting tokens
            Lexer lexer = lexer_new("test/lexer.txt");
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
            Lexer lexer = lexer_new("test/expr.txt");
            Expr expr = expr_parse(&lexer); 
            lexer_free(&lexer);
            expr_print(&expr, 0);
            expr_free(&expr);
        }

        putchar('\n');

        { // test parsing scopes
            Lexer lexer = lexer_new("test/scope.txt");
            Scope scope = scope_parse(&lexer);
            lexer_free(&lexer);
            scope_print(&scope, 0);
            scope_free(&scope);
        }

        /*
        putchar('\n');

        { // test parsing declarations
            char *str = "test/declaration.creed";
            char *str_copy = malloc(strlen(str) + sizeof(char));
            strcpy(str_copy, str);
            StringId path = string_cache_insert(str_copy);
            
            printf("source file path: %s\n", string_cache_get(path));
            
            Project project = project_new(path);
            SourceFile *file = project_get(&project, path);
            assert(file);
            source_file_print(file);
            project_free(&project);
        }


        putchar('\n');


        { // test typechecking
            SymbolTable table = symbol_table_from_file("test/symbol_table.creed");
            symbol_table_check_functions(&table); 
            symbol_table_free_head(&table);
        }
        */
    }

    string_cache_free();
    return EXIT_SUCCESS;
}
