#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "handlers.h"
#include "parser.h"
#include "string_cache.h"
#include "symbol_table.h"

SymbolTable * symbol_table;

// TO DO: Figure out how to parse identifier for arrays
const char * get_complex_type(Type creadz_type, char * c_prim_type) {
    char * c_type;
    if (creadz_type.type == (TYPE_PTR | TYPE_PTR_NULLABLE)) {
        c_type = c_prim_type;
        strcat(c_type, " *");
    }
    else {
        c_type = c_prim_type;
    }
    return c_type;
}

const char * get_type(Type creadz_type) {
    TokenType creadz_prim_type = creadz_type.data.primitive;
    char * c_prim_type;

    switch(creadz_prim_type) {
        case TOKEN_KEYWORD_TYPE_VOID:
        case TOKEN_KEYWORD_TYPE_CHAR:
        case TOKEN_KEYWORD_TYPE_INT:
        case TOKEN_KEYWORD_TYPE_FLOAT:
            c_prim_type = (string_keywords[creadz_prim_type - TOKEN_KEYWORD_MIN]);
            break;

        case TOKEN_KEYWORD_TYPE_INT8:
            c_prim_type = "char";
            break;
        case TOKEN_KEYWORD_TYPE_INT16:
            c_prim_type = "short";
            break;
        case TOKEN_KEYWORD_TYPE_INT64:
            c_prim_type = "long long";
            break;
        case TOKEN_KEYWORD_TYPE_UINT8:
            c_prim_type = "unsigned char";
            break;
        case TOKEN_KEYWORD_TYPE_UINT16:
            c_prim_type = "unsigned short";
            break;
        case TOKEN_KEYWORD_TYPE_UINT:
            c_prim_type = "unsigned int";
            break;
        case TOKEN_KEYWORD_TYPE_UINT64:
            c_prim_type = "unsigned long long";
            break;
        case TOKEN_KEYWORD_TYPE_FLOAT64:
            c_prim_type = "double";
            break;
        case TOKEN_KEYWORD_TYPE_BOOL:
            c_prim_type = "int";
            break;
        default:
            assert(false);
    }

    if (creadz_type.type != TYPE_PRIMITIVE) {
        return get_complex_type(creadz_type, c_prim_type);
    }

    return c_prim_type;
}

// TO DO: Add literal printing
void handle_literals(Literal literal) {

}

// TO DO: Add scope handling
void handle_scope(Scope * scope) {

}

void handle_expr(Expr * expr) {
    switch(expr->type) {
        case EXPR_PAREN:
            putchar(TOKEN_PAREN_OPEN);
            handle_expr(expr->data.parenthesized);
            putchar(TOKEN_PAREN_CLOSE);
            break;
        
        case EXPR_UNARY:
            putchar(expr->data.unary.type);
            handle_expr(expr->data.unary.operand);
            break;

        case EXPR_BINARY:
            handle_expr(expr->data.binary.lhs);
            printf(" %s ", string_operators[expr->data.binary.operator - TOKEN_OP_MIN]);
            handle_expr(expr->data.binary.rhs);
            break;

        case EXPR_TYPECAST:
            putchar(TOKEN_PAREN_OPEN);
            const char * type = get_type(expr->data.typecast.cast_to);
            printf("%s", type);
            putchar(TOKEN_PAREN_CLOSE);
            handle_expr(expr->data.typecast.operand);
            break;

        case EXPR_ACCESS_MEMBER:
            handle_expr(expr->data.access_member.operand);
            putchar(TOKEN_DOT);
            printf("%s", string_cache_get(expr->data.access_member.member));
            break;

        case EXPR_ACCESS_ARRAY:
            handle_expr(expr->data.access_array.operand);
            putchar(TOKEN_BRACKET_OPEN);
            handle_expr(expr->data.access_array.index);
            putchar(TOKEN_BRACKET_CLOSE);
            break;

        case EXPR_FUNCTION:
            putchar(TOKEN_PAREN_OPEN);
            if (expr->data.function.param_count > 0) { 
                for (int i = 0; i < expr->data.function.param_count - 1; i++) {
                    const char * param_type = get_type(expr->data.function.params[i].type);
                    printf("%s ", param_type);
                    printf("%s", string_cache_get(expr->data.function.params[i].id));
                    printf("%c ", TOKEN_COMMA);
                }
                const char * param_type = get_type(expr->data.function.params[expr->data.function.param_count - 1].type);
                printf("%s ", param_type);
                printf("%s", string_cache_get(expr->data.function.params[expr->data.function.param_count - 1].id));
            }
            putchar(TOKEN_PAREN_CLOSE);
            handle_scope(expr->data.function.scope);
            break;

        case EXPR_FUNCTION_CALL:
            handle_expr(expr->data.function_call.function);
            putchar(TOKEN_PAREN_OPEN);
            if (expr->data.function_call.param_count > 0) {
                int last_param_idx = expr->data.function_call.param_count - 1;
                for (int i = 0; i < last_param_idx; i++) {
                    handle_expr(&expr->data.function_call.params[i]);
                    printf("%c ", TOKEN_COMMA);
                }
                handle_expr(&expr->data.function_call.params[last_param_idx]);
            }
            putchar(TOKEN_PAREN_CLOSE);
            break;

        case EXPR_ID:
            printf("%s", string_cache_get(expr->data.id));
            break;

        case EXPR_LITERAL:
            handle_literals(expr->data.literal);
            break;

        case EXPR_LITERAL_BOOL:
            if (expr->data.literal_bool == false) {
                printf("%d", 0);
            }
            else if (expr->data.literal_bool == true) {
                printf("%d", 1);
            }
            break;
    }   
}

// TO DO: Only print semicolons after scopes that are declarations??
void handle_statement_end() {
    printf(";\n");
}
