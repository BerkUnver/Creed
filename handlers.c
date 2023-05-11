#include <assert.h>
#include "handlers.h"
#include "parser.h"
#include "string_cache.h"

/*
SymbolTable * symbol_table;

char * get_complex_type(Type creadz_type, char * c_prim_type) {
    return c_prim_type;     // TO DO: Fix this to ouput correct C format for arrays and ptrs
}

char * get_type(Type creadz_type) {
    TokenType creadz_prim_type = creadz_type.data.primitive;
    char * c_prim_type;

    switch(creadz_prim_type) {
        case TOKEN_KEYWORD_TYPE_VOID:
        case TOKEN_KEYWORD_TYPE_CHAR:
        case TOKEN_KEYWORD_TYPE_INT:
        case TOKEN_KEYWORD_TYPE_FLOAT:
            c_prim_type = (string_keywords[creadz_prim_type - TOKEN_KEYWORD_MIN]);

        case TOKEN_KEYWORD_TYPE_INT8:
            c_prim_type = "char";
        case TOKEN_KEYWORD_TYPE_INT16:
            c_prim_type = "short";
        case TOKEN_KEYWORD_TYPE_INT64:
            c_prim_type = "long long";
        case TOKEN_KEYWORD_TYPE_UINT8:
            c_prim_type = "unsigned char";
        case TOKEN_KEYWORD_TYPE_UINT16:
            c_prim_type = "unsigned short";
        case TOKEN_KEYWORD_TYPE_UINT:
            c_prim_type = "unsigned int";
        case TOKEN_KEYWORD_TYPE_UINT64:
            c_prim_type = "unsigned long long";
        case TOKEN_KEYWORD_TYPE_FLOAT64:
            c_prim_type = "double";
        case TOKEN_KEYWORD_TYPE_BOOL:
            c_prim_type = "int";
        default:
            assert(false);
    }

    if (creadz_type.type != TYPE_PRIMITIVE) {
        return get_complex_type(creadz_type, c_prim_type);
    }

    return c_prim_type;
}

void handle_arithmetic_expr(Expr * expr) {
    if (expr->type == EXPR_BINARY) {
        expr_print(expr);
    }
}

void handle_function_declaration(Declaration * func) {
    // Commented out to avoid warnings
    // int num_params = func->data.d_function.parameter_count;
    // Type creadz_type = func->data.d_function.return_type;
    // char * id = string_cache_get(func->id);
    // char * c_type = get_type(creadz_type);
}
*/

void handle_semicolon() {
    printf(";\n");      // TO DO: Only print semicolons after scopes that are declarations??
}
