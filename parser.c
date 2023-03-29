#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "print.h"
#include "token.h"

Type type_parse(Lexer *lexer) {
    Token token_id = lexer_token_get(lexer);
    if (token_id.type != TOKEN_ID) {
        error_exit(token_id.location, "Expected a type id at the beginning of a type signature.");
    }
    
    Type type = {
        .location = token_id.location,
        .type = TYPE_ID,
        .data.id = token_id.data.id
    };

    while (true) {
        int ptr_type; // I don't wanna make the type type a named type just for this
        
        switch (lexer_token_peek(lexer).type) {
            case TOKEN_OP_MULTIPLY:
                ptr_type = TYPE_PTR;
                break;

            case TOKEN_QUESTION_MARK:
                ptr_type = TYPE_PTR_NULLABLE;
                break;
            
            case TOKEN_BRACKET_OPEN: {
                Token token_open = lexer_token_get(lexer);
                if (lexer_token_peek(lexer).type != TOKEN_BRACKET_CLOSE) {
                    Location location = location_expand(type.location, token_open.location);
                    error_exit(location, "Expected a closing bracket after the opening bracket of an array declaration.");
                }
                
                ptr_type = TYPE_ARRAY;
            } break;
            
            default:
                return type;
        }
        
        Type *sub_type = malloc(sizeof(Type));
        *sub_type = type;
        
        Token token_end = lexer_token_get(lexer);
        type = (Type) {
            .location = location_expand(type.location, token_end.location),
            .type = ptr_type,
            .data.sub_type = sub_type
        };
    }
}

void type_free(Type *type) {
    switch (type->type) {   
        case TYPE_PTR:
        case TYPE_PTR_NULLABLE:
        case TYPE_ARRAY:
            type_free(type->data.sub_type);
            free(type->data.sub_type);
            break;

        default: break;
    }
}

void type_print(Type *type) {
    switch (type->type) {
        case TYPE_ID:
            print(string_cache_get(type->data.id));
            break;
        
        case TYPE_PTR:
            type_print(type->data.sub_type);
            print(string_operators[TOKEN_OP_MULTIPLY - TOKEN_OP_MIN]);
            break;
        
        case TYPE_PTR_NULLABLE:
            type_print(type->data.sub_type);
            putchar(TOKEN_QUESTION_MARK);
            break;

        case TYPE_ARRAY:
            type_print(type->data.sub_type);
            putchar(TOKEN_BRACKET_OPEN);
            putchar(TOKEN_BRACKET_CLOSE);
            break;
    }
}

static Expr expr_parse_precedence(Lexer *lexer, int precedence) {
    Expr expr;
    switch (lexer_token_peek(lexer).type) {
        case TOKEN_LITERAL: {
            Token token = lexer_token_get(lexer);
            expr.type = EXPR_LITERAL;
            expr.location = token.location;
            expr.data.literal = token.data.literal;
        } break;
      
        case TOKEN_PAREN_OPEN: {
            Token token_open = lexer_token_get(lexer);
            Expr operand = expr_parse(lexer);
            
            if (lexer_token_peek(lexer).type != TOKEN_PAREN_CLOSE) {
                Location location = location_expand(token_open.location, operand.location);
                error_exit(location, "Expected a closing parenthesis at the end of a parenthesized expression."); 
            }        

            Token token_close = lexer_token_get(lexer);
            Expr *operand_ptr = malloc(sizeof(Expr));
            *operand_ptr = operand;
            
            expr = (Expr) {
                .location = location_expand(token_open.location, token_close.location),
                .type = EXPR_UNARY,
                .data.unary.operator = EXPR_UNARY_PAREN,
                .data.unary.operand = operand_ptr
            };
        } break;

        case TOKEN_ID: {
            Token token_id = lexer_token_get(lexer);
            expr.type = EXPR_ID;
            expr.data.id = token_id.data.id;
            expr.location = token_id.location;
        } break;
        
        default: error_exit(lexer_token_get(lexer).location, "Expected an expression here.");
    }
    
    while (true) {     
        TokenType op_type = lexer_token_peek(lexer).type;
        if (op_type == TOKEN_KEYWORD_TYPECAST) {
            lexer_token_get(lexer);
            
            Type cast_to = type_parse(lexer); // TODO: error handling
            Expr *operand = malloc(sizeof(Expr));
            *operand = expr;
            expr = (Expr) {
                .location = location_expand(expr.location, cast_to.location),
                .type = EXPR_TYPECAST,
                .data.typecast.operand = operand,
                .data.typecast.cast_to = cast_to
            };
            continue;
        
        } else if (op_type == TOKEN_PAREN_OPEN) { // function call
            lexer_token_get(lexer); 
            int param_count = 0;
            Expr *params = NULL;
            
            if (lexer_token_peek(lexer).type != TOKEN_PAREN_CLOSE) {
                while (true) {
                    Expr param = expr_parse(lexer);
                    
                    param_count++;
                    params = realloc(params, sizeof(Expr) * param_count);
                    params[param_count - 1] = param;
                    
                    if (lexer_token_peek(lexer).type == TOKEN_PAREN_CLOSE) break;
                    if (lexer_token_peek(lexer).type == TOKEN_COMMA) {
                        lexer_token_get(lexer);
                    } else {
                        error_exit(expr.location, "Expected a comma after a function parameter.");
                    }    
                }
            }
            
            Token paren_close = lexer_token_get(lexer);
            Expr *function = malloc(sizeof(Expr));
            *function = expr;
            expr.type = EXPR_FUNCTION_CALL;
            expr.data.function_call.function = function;
            expr.data.function_call.params = params;
            expr.data.function_call.param_count = param_count;
            expr.location = location_expand(expr.location, paren_close.location);
            continue;
        }

        if (op_type < TOKEN_OP_MIN || TOKEN_OP_MAX < op_type) return expr;
        
        int op_precedence = operator_precedences[op_type - TOKEN_OP_MIN];
        if (op_precedence < precedence) return expr; 
        
        lexer_token_get(lexer); // get the operator

        Expr *lhs = malloc(sizeof(Expr));
        *lhs = expr;
        
        Expr *rhs = malloc(sizeof(Expr));
        *rhs = expr_parse_precedence(lexer, op_precedence);
        
        expr = (Expr) {
            .location = location_expand(expr.location, rhs->location), 
            .type = EXPR_BINARY,
            .data.binary.operator = op_type,
            .data.binary.lhs = lhs,
            .data.binary.rhs = rhs,
        };
    }
}

Expr expr_parse(Lexer *lexer) {
    return expr_parse_precedence(lexer, 0);
}

void expr_free(Expr *expr) {
    // currently only frees things that are actually implemented.
    switch (expr->type) {
        case EXPR_UNARY:
            expr_free(expr->data.unary.operand);
            free(expr->data.unary.operand);
            break;

        case EXPR_BINARY:
            expr_free(expr->data.binary.lhs);
            free(expr->data.binary.lhs);
            expr_free(expr->data.binary.rhs);
            free(expr->data.binary.rhs);
            break;
    
        case EXPR_TYPECAST:
            expr_free(expr->data.typecast.operand);
            free(expr->data.typecast.operand);
            type_free(&expr->data.typecast.cast_to);
            break;
        
        case EXPR_FUNCTION_CALL:
            for (int i = 0; i < expr->data.function_call.param_count; i++) {
                expr_free(&expr->data.function_call.params[i]);
            }
            free(expr->data.function_call.params);
            expr_free(expr->data.function_call.function);
            free(expr->data.function_call.function);
            break;

        default: break;
    }
}

void expr_print(Expr *expr) {
    
    // currently only prints things that are actually implemented.
    switch (expr->type) {
        case EXPR_UNARY:
            putchar('(');
            expr_print(expr->data.unary.operand);
            putchar(')');
            break;
        
        case EXPR_BINARY:
            putchar('('); // these are for debug purposes to make sure operator precedence is working properly. Remove?
            expr_print(expr->data.binary.lhs);
            printf(" %s ", string_operators[expr->data.binary.operator - TOKEN_OP_MIN]);
            expr_print(expr->data.binary.rhs);
            putchar(')');
            break;
        
        case EXPR_ID:
            print(string_cache_get(expr->data.id));
            break;

        case EXPR_LITERAL:
            literal_print(&expr->data.literal);
            break;
        
        case EXPR_TYPECAST:
            expr_print(expr->data.typecast.operand);
            printf(" %s ", string_keywords[TOKEN_KEYWORD_TYPECAST - TOKEN_KEYWORD_MIN]);
            type_print(&expr->data.typecast.cast_to);
            break;
        
        case EXPR_FUNCTION_CALL:
            expr_print(expr->data.function_call.function);
            putchar(TOKEN_PAREN_OPEN);
            if (expr->data.function_call.param_count > 0) {
                int last_param_idx = expr->data.function_call.param_count - 1;
                for (int i = 0; i < last_param_idx; i++) {
                    expr_print(&expr->data.function_call.params[i]);
                    printf("%c ", TOKEN_COMMA);
                }
                expr_print(&expr->data.function_call.params[last_param_idx]);
            }
            putchar(TOKEN_PAREN_CLOSE);
            break;
    }
}

Statement statement_parse(Lexer *lexer) {
    // TODO: add conditionals, loops, ect. (tom)
    Token token_var = lexer_token_peek(lexer);

    if (token_var.type != TOKEN_ID) {
        error_exit(token_var.location, "Expected a variable declaration statement to start with the name of the variable.");
    }
    lexer_token_get(lexer);

    if (lexer_token_peek(lexer).type != TOKEN_COLON) {
        error_exit(token_var.location, "Expected a colon after a variable name in a variable declaration."); 
    }
    lexer_token_get(lexer);

    Statement statement;

    Type type = type_parse(lexer);
    if (lexer_token_peek(lexer).type == TOKEN_ASSIGN) {
        lexer_token_get(lexer);
        statement.data.var_declare.assign = expr_parse(lexer);
        statement.data.var_declare.has_assign = true;
    } else {
        statement.data.var_declare.has_assign = false;
    }

    if (lexer_token_peek(lexer).type != TOKEN_SEMICOLON) {
        error_exit(location_expand(token_var.location, type.location), "Expected a semicolon after a variable declaration.");
    }
    Token token_semicolon = lexer_token_get(lexer); 
    
    statement.location = location_expand(token_var.location, token_semicolon.location);
    statement.type = STATEMENT_VAR_DECLARE;
    statement.data.var_declare.id = token_var.data.id;
    statement.data.var_declare.type = type;
    return statement;
}

void statement_free(Statement *statement) {
    switch (statement->type) {
        case STATEMENT_VAR_DECLARE:
            type_free(&statement->data.var_declare.type);
            if (statement->data.var_declare.has_assign) {
                expr_free(&statement->data.var_declare.assign);
            }
            break;
    }
}

void statement_print(Statement *statement) {
    switch (statement->type) {
        case STATEMENT_VAR_DECLARE:
            print(string_cache_get(statement->data.var_declare.id));
            putchar(TOKEN_COLON);
            putchar(' ');
            type_print(&statement->data.var_declare.type);
            if (statement->data.var_declare.has_assign) {
                printf(" = ");
                expr_print(&statement->data.var_declare.assign);
            }
            putchar(TOKEN_SEMICOLON);
            break;
    }
}

Scope scope_parse(Lexer *lexer) {
    Token token_start = lexer_token_peek(lexer);
    if (token_start.type != TOKEN_CURLY_BRACE_OPEN) {
        error_exit(token_start.location, "Expected an opening curly brace at the beginning of a scope.");
    }
    lexer_token_get(lexer);

    int statement_count = 0;
    Statement *statements = NULL;

    while (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
        statement_count++;
        statements = realloc(statements, sizeof(Statement) * statement_count);
        statements[statement_count - 1] = statement_parse(lexer);
    }

    Token token_end = lexer_token_get(lexer);
    
    return (Scope) {
        .location = location_expand(token_start.location, token_end.location),
        .statement_count = statement_count,
        .statements = statements
    };
}

void scope_free(Scope *scope) {
    for (int i = 0; i < scope->statement_count; i++) {
        statement_free(scope->statements + i);
    }
    free(scope->statements);
}

void scope_print(Scope *scope, int indentation) {
    if (scope->statement_count == 0) {
        printf("%c %c", TOKEN_CURLY_BRACE_OPEN, TOKEN_CURLY_BRACE_CLOSE);
        return;
    }
    
    putchar(TOKEN_CURLY_BRACE_OPEN);
    putchar('\n');
    for (int i = 0; i < scope->statement_count; i++) {
        for (int j = 0; j < indentation + 1; j++) {
            print(STR_INDENTATION);
        }
        statement_print(&scope->statements[i]);
        putchar('\n');
    }
    for (int i = 0; i < indentation - 1; i++) {
        print(STR_INDENTATION);
    }
    putchar(TOKEN_CURLY_BRACE_CLOSE);
    putchar('\n');
}
