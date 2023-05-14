#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "handlers.h"
#include "parser.h"
#include "string_cache.h"
#include "symbol_table.h"

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
    assert(creadz_type.type == TYPE_PRIMITIVE);
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

void handle_literals(Literal * literal) {
    switch (literal->type) {
        case LITERAL_STRING:
            putchar('"');
            int idx = 0;
            char * string = string_cache_get(literal->data.l_string);
            while (string[idx] != '\0') {
                print_literal_char(string[idx]);
                idx++;
            }
            putchar('"');
            break;
        case LITERAL_INT8:
            printf("%d", literal->data.l_int8);  
            break;     
        case LITERAL_INT16:
            printf("%hi", literal->data.l_int16);     
            break;  
        case LITERAL_INT:
            printf("%d", literal->data.l_int);     
            break;  
        case LITERAL_INT64:
            printf("%lldll", literal->data.l_int64);       
            break; 
        case LITERAL_UINT8:
            printf("%uu", literal->data.l_uint8);
            break;
        case LITERAL_UINT16:
            printf("%huu", literal->data.l_uint16);
            break;
        case LITERAL_UINT:
            printf("%uu", literal->data.l_uint);
            break;
        case LITERAL_UINT64:
            printf("%lluull", literal->data.l_uint64);
            break;
        case LITERAL_FLOAT:
            printf("%ff", literal->data.l_float);
            break;
        case LITERAL_FLOAT64:
            printf("%lf", literal->data.l_float64);
            break;
        case LITERAL_CHAR:
            printf("%c", literal->data.l_char);
            break;
    }
}

// TO DO: Add statement handling
void handle_statement(Statement * statement) {

}

void handle_scope(Scope * scope) {
    switch(scope->type) {
        case SCOPE_STATEMENT:
            handle_statement(&scope->data.statement);
            handle_statement_end();
            break;

        case SCOPE_CONDITIONAL:
            printf("if (");
            handle_expr(&scope->data.conditional.condition);
            printf(") ");
            handle_scope(scope->data.conditional.scope_if);
            if (scope->data.conditional.scope_else) {
                printf("\nelse ");
                handle_scope(scope->data.conditional.scope_else);
            }
            break;
            
        case SCOPE_LOOP_FOR:
            printf("for (");
            handle_statement(&scope->data.loop_for.init);
            printf("%c ", TOKEN_SEMICOLON);
            handle_expr(&scope->data.loop_for.expr);
            printf("%c ", TOKEN_SEMICOLON);
            handle_statement(&scope->data.loop_for.step);
            printf(") ");
            handle_scope(scope->data.loop_for.scope);
            break;
            
        // TO DO: Get size of array for iteration
        case SCOPE_LOOP_FOR_EACH:
            break;
            
        case SCOPE_LOOP_WHILE:
            printf("while (");
            handle_expr(&scope->data.loop_while.expr);
            printf(")");
            handle_scope(scope->data.loop_while.scope);
            break;

        case SCOPE_BLOCK:
            printf(" {\n");
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                handle_scope(&scope->data.block.scopes[i]);
                putchar('\n');
            }
            putchar(TOKEN_CURLY_BRACE_CLOSE);

        // TO DO: Handle match statements?
        case SCOPE_MATCH:
            break;
    }
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
            // Berk: Not guaranteed to be a primitive type, could be a pointer.
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
            // Berk comment
            //
            // If I'm understanding this correctly you're just translating the function call to look like a c-style function.
            // Because we allow anonymous functions, that doesn't really work because you need to also generate a unique name for each function.
            // I commented this out because I changed how functions are represented in the AST and this gives compilation errors.
            
            putchar(TOKEN_PAREN_OPEN);
            if (expr->data.function.type.data.function.param_count > 0) {
                for (int i = 0; i < expr->data.function.type.data.function.param_count - 1; i++) {
                    const char * param_type = get_type(expr->data.function.type.data.function.params[i].type);
                    printf("%s ", param_type);
                    printf("%s", string_cache_get(expr->data.function.type.data.function.params[i].id));
                    printf("%c ", TOKEN_COMMA);
                }
                const char * param_type = get_type(expr->data.function.type.data.function.params[expr->data.function.type.data.function.param_count - 1].type);
                printf("%s ", param_type);
                printf("%s", string_cache_get(expr->data.function.type.data.function.params[expr->data.function.type.data.function.param_count - 1].id));
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
            handle_literals(&expr->data.literal);
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

// TO DO: Only print semicolons after scopes that are statements
void handle_statement_end() {
    printf(";\n");
}

void handle_declaration(Declaration * declaration) {
    switch(declaration->type) {

        case DECLARATION_VAR:
            switch (declaration->data.var.type) {
                case DECLARATION_VAR_CONSTANT: {
                    // Berk comment: TODO: account for type inference of constants.
                    const char * type = get_type(declaration->data.var.data.constant.data.type_implicit);
                    if (declaration->data.var.data.constant.type == DECLARATION_VAR_CONSTANT_TYPE_EXPLICIT) {
                        type = get_type(declaration->data.var.data.constant.data.type_explicit);
                    }
                    printf("%s ", type);
                    if (declaration->id.idx) {
                        const char * id = string_cache_get(declaration->id);
                        printf("%s", id);
                    } 
                    else {
                        // TO DO: Allow for anonymous functions (declaring functions with no names)?
                        error_exit(declaration->location, "Function declaration must include an identifier.");
                    }
                    handle_expr(&declaration->data.var.data.constant.value);
                    break;
                }
                case DECLARATION_VAR_MUTABLE: {
                    const char * type = get_type(declaration->data.var.data.mutable.type);
                    const char * id = string_cache_get(declaration->id);
                    printf("%s %s", type, id);
                    handle_expr(&declaration->data.var.data.constant.value);
                    if (declaration->data.var.data.mutable.value_exists) {
                        printf(" %s ", string_assigns[TOKEN_ASSIGN - TOKEN_ASSIGN_MIN]);
                        handle_expr(&declaration->data.var.data.constant.value);
                    }                   
                    break;
                }
            }
            break;

        // TO DO: Implement assigning a value to an enum?
        case DECLARATION_ENUM:
            printf("enum %s %c\n", string_cache_get(declaration->id), TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < declaration->data.enumeration.member_count; i++) {
                printf("%s%c\n", string_cache_get(declaration->data.enumeration.members[i]), TOKEN_COMMA);
            }
            printf("%c%c", TOKEN_CURLY_BRACE_CLOSE, TOKEN_SEMICOLON);
            break;

        case DECLARATION_STRUCT:
            printf("struct %s %c\n", string_cache_get(declaration->id), TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < declaration->data.struct_union.member_count; i++) {
                const char * member_type = get_type(declaration->data.struct_union.members[i].type);
                char * member_id = string_cache_get(declaration->data.struct_union.members[i].id);
                printf("%s %s%c\n", member_type, member_id, TOKEN_SEMICOLON);
            }
            printf("%c%c", TOKEN_CURLY_BRACE_CLOSE, TOKEN_SEMICOLON);
            break;

        case DECLARATION_UNION:
            printf("union %s %c\n", string_cache_get(declaration->id), TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < declaration->data.struct_union.member_count; i++) {
                const char * member_type = get_type(declaration->data.struct_union.members[i].type);
                char * member_id = string_cache_get(declaration->data.struct_union.members[i].id);
                printf("%s %s%c\n", member_type, member_id, TOKEN_SEMICOLON);
            }
            printf("%c%c", TOKEN_CURLY_BRACE_CLOSE, TOKEN_SEMICOLON);
            break;

        // TO DO: Implement sum types?
        case DECLARATION_SUM:
            error_exit(declaration->location, "Translating this kind of declaration into C has not been implemented yet.");
            break;
    }
}

// TO DO: Have this called somewhere after SourceFile is parsed
void handle_driver(SourceFile * file) {
    // First pass
    for (int i = 0; i < file->declaration_count; i++) {
        if (file->declarations[i].type != DECLARATION_VAR) {
            handle_declaration(&file->declarations[i]);
        }
    }
    // Second Pass
    for (int j = 0; j < file->declaration_count; j++) {
        if (file->declarations[j].type == DECLARATION_VAR) {
            handle_declaration(&file->declarations[j]);
        }
    }
}   
