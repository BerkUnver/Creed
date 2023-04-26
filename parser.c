#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "prelude.h"
#include "token.h"

Type type_parse(Lexer *lexer) {
    Token token = lexer_token_peek(lexer);

    int type_modifier = 0;
    switch (token.type) {
        case TOKEN_OP_MULTIPLY:
            type_modifier = TYPE_PTR; 
            break;

        case TOKEN_QUESTION_MARK:
            type_modifier = TYPE_PTR_NULLABLE;
            break;

        case TOKEN_BRACKET_OPEN: {
            lexer_token_get(lexer);
            if (lexer_token_peek(lexer).type != TOKEN_BRACKET_CLOSE) {
                error_exit(token.location, "Expected a closing bracket after the opening bracket of an array declaration.");
            } 
            type_modifier = TYPE_ARRAY;
        } break;

        case TOKEN_ID:
            lexer_token_get(lexer);
            return (Type) {
                .location = token.location,
                .type = TYPE_ID,
                .data.id = token.data.id
            };
        
        default:
            if (token.type < TOKEN_KEYWORD_TYPE_MIN || TOKEN_KEYWORD_TYPE_MAX < token.type) {
                error_exit(token.location, "Expected type signature here.");
            }
            lexer_token_get(lexer);
            return (Type) {
                .location = token.location,
                .type = TYPE_PRIMITIVE,
                .data.primitive = token.type
            };
    }

    lexer_token_get(lexer);
    Type *sub_type = malloc(sizeof(Type));
    *sub_type = type_parse(lexer);
    
    return (Type) {
        .location = location_expand(token.location, sub_type->location),
        .type = type_modifier,
        .data.sub_type = sub_type 
    };
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
            print(string_operators[TOKEN_OP_MULTIPLY - TOKEN_OP_MIN]);
            type_print(type->data.sub_type);
            break;
        
        case TYPE_PTR_NULLABLE:
            putchar(TOKEN_QUESTION_MARK);
            type_print(type->data.sub_type);
            break;

        case TYPE_ARRAY:
            putchar(TOKEN_BRACKET_OPEN);
            putchar(TOKEN_BRACKET_CLOSE);
            type_print(type->data.sub_type);
            break;

        case TYPE_PRIMITIVE:
            print(string_keywords[type->data.primitive - TOKEN_KEYWORD_MIN]);
            // TODO: Add printing primitives here.
            break;
    }
}

static Expr expr_parse_modifiers(Lexer *lexer) { // parse unary operators, function calls, and member accesses
    Expr expr;
    switch (lexer_token_peek(lexer).type) {
        case TOKEN_LITERAL: {
            Token token = lexer_token_get(lexer);
            expr.type = EXPR_LITERAL;
            expr.location = token.location;
            expr.data.literal = token.data.literal;
        } break;
     
        int unary_type;
        case TOKEN_PAREN_OPEN: {
            Token token_open = lexer_token_get(lexer);

            Expr *parenthesized = malloc(sizeof(Expr));
            *parenthesized = expr_parse(lexer);
            
            if (lexer_token_peek(lexer).type != TOKEN_PAREN_CLOSE) {
                Location location = location_expand(token_open.location, parenthesized->location);
                error_exit(location, "Expected a closing parenthesis at the end of a parenthesized expression."); 
            }        
            Token token_close = lexer_token_get(lexer);
            
            expr = (Expr) {
                .location = location_expand(token_open.location, token_close.location),
                .type = EXPR_PAREN,
                .data.parenthesized = parenthesized
            };
        } break;

        case TOKEN_ID: {
            Token token_id = lexer_token_get(lexer);
            expr.type = EXPR_ID;
            expr.data.id = token_id.data.id;
            expr.location = token_id.location;
        } break;
        
        case TOKEN_KEYWORD_FALSE:
        case TOKEN_KEYWORD_TRUE: {
            Token token_bool = lexer_token_get(lexer);
            expr.location = token_bool.location;
            expr.type = EXPR_LITERAL_BOOL;
            expr.data.literal_bool = token_bool.type - TOKEN_KEYWORD_FALSE;
        } break;

        case TOKEN_UNARY_LOGICAL_NOT:
            unary_type = EXPR_UNARY_LOGICAL_NOT;
            goto unary;
        
        case TOKEN_BITWISE_NOT:
            unary_type = EXPR_UNARY_BITWISE_NOT;
            goto unary;
        
        case TOKEN_OP_MINUS:
            unary_type = EXPR_UNARY_NEGATE;
            goto unary;

        case TOKEN_OP_MULTIPLY:
            unary_type = EXPR_UNARY_DEREF;
            goto unary;

        case TOKEN_OP_BITWISE_AND: {
            unary_type = EXPR_UNARY_REF;

            unary: {
                Token token_not = lexer_token_get(lexer);
                Expr *operand = malloc(sizeof(Expr));
                *operand = expr_parse_modifiers(lexer);
                return (Expr) {
                    .type = EXPR_UNARY,
                    .data.unary.type = unary_type,
                    .data.unary.operand = operand,
                    .location = location_expand(token_not.location, operand->location)
                };
            }
        }

        default: error_exit(lexer_token_get(lexer).location, "Expected an expression here.");
    }
    
    // parse function calls and member accesses.
    while (true) {
        if (lexer_token_peek(lexer).type == TOKEN_PAREN_OPEN) {
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
        
        } else if (lexer_token_peek(lexer).type == TOKEN_DOT) {
            lexer_token_get(lexer);
            Token token_id = lexer_token_get(lexer);
            if (token_id.type != TOKEN_ID) {
                error_exit(token_id.location, "Expected the name of a member in a member-access expression.");
            }

            Expr *operand = malloc(sizeof(Expr));
            *operand = expr;

            expr.type = EXPR_ACCESS_MEMBER;
            expr.data.access_member.operand = operand;
            expr.data.access_member.member = token_id.data.id;
            expr.location = location_expand(expr.location, token_id.location);
        
        } else if (lexer_token_peek(lexer).type == TOKEN_BRACKET_OPEN) { 
            lexer_token_get(lexer);
            Expr *index = malloc(sizeof(Expr));
            *index = expr_parse(lexer);
            if (lexer_token_peek(lexer).type != TOKEN_BRACKET_CLOSE) {
                error_exit(index->location, "Expected a closing bracket at the end of an array access.");
            }
            Token token_end = lexer_token_get(lexer);
            Expr *operand = malloc(sizeof(Expr));
            *operand = expr;
            expr = (Expr) {
                .type = EXPR_ACCESS_ARRAY,
                .data.access_array.operand = operand,
                .data.access_array.index = index,
                .location = location_expand(expr.location, token_end.location) 
            };
        } else break;
    }

    return expr;
}

static Expr expr_parse_precedence(Lexer *lexer, int precedence) {

    Expr expr = expr_parse_modifiers(lexer);
    
    // parse typecasts
    while (lexer_token_peek(lexer).type == TOKEN_KEYWORD_TYPECAST) {
        lexer_token_get(lexer);
        Type type = type_parse(lexer);
        
        Expr *operand = malloc(sizeof(Expr));
        *operand = expr;

        expr = (Expr) {
            .location = location_expand(expr.location, type.location),
            .type = EXPR_TYPECAST,
            .data.typecast.operand = operand,
            .data.typecast.cast_to = type
        };
    }
   

    // parse operators
    while (true) {
        TokenType op_type = lexer_token_peek(lexer).type;
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

    return expr;
}

Expr expr_parse(Lexer *lexer) {
    return expr_parse_precedence(lexer, 0);
}

void expr_free(Expr *expr) {
    // currently only frees things that are actually implemented.
    switch (expr->type) {

        case EXPR_PAREN:
            expr_free(expr->data.parenthesized);
            free(expr->data.parenthesized);
            break;

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
        
        case EXPR_ACCESS_MEMBER:
            expr_free(expr->data.access_member.operand);
            free(expr->data.access_member.operand);
            break;

        case EXPR_ACCESS_ARRAY:
            expr_free(expr->data.access_array.operand);
            free(expr->data.access_array.operand);
            expr_free(expr->data.access_array.index);
            free(expr->data.access_array.index);
            break;
        
        case EXPR_FUNCTION_CALL:
            for (int i = 0; i < expr->data.function_call.param_count; i++) {
                expr_free(&expr->data.function_call.params[i]);
            }
            free(expr->data.function_call.params);
            expr_free(expr->data.function_call.function);
            free(expr->data.function_call.function);
            break;

        case EXPR_ID:
        case EXPR_LITERAL:
        case EXPR_LITERAL_BOOL:
            break;
    }
}

void expr_print(Expr *expr) { 
    switch (expr->type) {
        case EXPR_PAREN:
            putchar(TOKEN_PAREN_OPEN);
            expr_print(expr->data.parenthesized);
            putchar(TOKEN_PAREN_CLOSE);
            break;

        case EXPR_UNARY:
            putchar(expr->data.unary.type);
            expr_print(expr->data.unary.operand);
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
        
        case EXPR_LITERAL_BOOL:
            print(string_keywords[expr->data.literal_bool + TOKEN_KEYWORD_FALSE - TOKEN_KEYWORD_MIN]);
            break;

        case EXPR_TYPECAST:
            putchar('(');
            expr_print(expr->data.typecast.operand);
            printf(" %s ", string_keywords[TOKEN_KEYWORD_TYPECAST - TOKEN_KEYWORD_MIN]);
            type_print(&expr->data.typecast.cast_to);
            putchar(')');
            break;
        
        case EXPR_ACCESS_MEMBER:
            putchar('(');
            expr_print(expr->data.access_member.operand);
            putchar(')');
            putchar(TOKEN_DOT);
            print(string_cache_get(expr->data.access_member.member));
            break;

        case EXPR_ACCESS_ARRAY:
            expr_print(expr->data.access_array.operand);
            putchar(TOKEN_BRACKET_OPEN);
            expr_print(expr->data.access_array.index);
            putchar(TOKEN_BRACKET_CLOSE);
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
    if (lexer_token_peek_2(lexer).type == TOKEN_COLON) {
        Token token_id = lexer_token_peek(lexer);
        if (token_id.type != TOKEN_ID) {
            error_exit(token_id.location, "Expected the name of a variable to be an id.");
        }
        lexer_token_get(lexer);
        lexer_token_get(lexer); // get colon

        Statement statement;

        Type type = type_parse(lexer);
        if (lexer_token_peek(lexer).type == TOKEN_ASSIGN) {
            lexer_token_get(lexer);
            statement.data.var_declare.assign = expr_parse(lexer);
            statement.data.var_declare.has_assign = true;
        } else {
            statement.data.var_declare.has_assign = false;
        }

        statement.location = location_expand(token_id.location, type.location);
        statement.type = STATEMENT_VAR_DECLARE;
        statement.data.var_declare.id = token_id.data.id;
        statement.data.var_declare.type = type;
        return statement;
    }

    switch (lexer_token_peek(lexer).type) {
        case TOKEN_INCREMENT: {
            Token token_increment = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            return (Statement) {
                .location = location_expand(token_increment.location, expr.location),
                .type = STATEMENT_INCREMENT,
                .data.increment = expr
            };
        }

        case TOKEN_DEINCREMENT: {
            Token token_deincrement = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            return (Statement) {
                .location = location_expand(token_deincrement.location, expr.location),
                .type = STATEMENT_DEINCREMENT,
                .data.deincrement = expr
            };
        }
        
        case TOKEN_KEYWORD_LABEL: {
            Token token_label = lexer_token_get(lexer);
            Token token_id = lexer_token_peek(lexer);
            if (token_id.type != TOKEN_ID) {
                error_exit(token_id.location, "Expected an identifier as a label.");
            }
            lexer_token_get(lexer);
            return (Statement) {
                .location = location_expand(token_label.location, token_id.location),
                .type = STATEMENT_LABEL,
                .data.label = token_id.data.id
            };
            break;
        }
        
        case TOKEN_KEYWORD_GOTO: {
            Token token_goto = lexer_token_get(lexer);
            Token token_id = lexer_token_peek(lexer);
            if (token_id.type != TOKEN_ID) error_exit(token_id.location, "Expected the label of a goto statement to be an identifier.");
            lexer_token_get(lexer);
            return (Statement) {
                .location = location_expand(token_goto.location, token_id.location),
                .type = STATEMENT_LABEL_GOTO,
                .data.label_goto = token_id.data.id
            };
            break;
        }

        case TOKEN_KEYWORD_RETURN: {
            Token token_return = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            return (Statement) {
                .location = location_expand(token_return.location, expr.location),
                .type = STATEMENT_RETURN,
                .data.return_expr = expr, 
            };
        }

        default: {
            Expr expr = expr_parse(lexer);
            Token token_assign = lexer_token_peek(lexer);
            
            if (token_assign.type < TOKEN_ASSIGN_MIN || TOKEN_ASSIGN_MAX < token_assign.type) {
                return (Statement) {
                    .location = expr.location,
                    .type = STATEMENT_EXPR,
                    .data.expr = expr
                };
            } else {
                lexer_token_get(lexer);
                Expr value = expr_parse(lexer);
                return (Statement) {
                    .location = location_expand(expr.location, expr.location),
                    .type = STATEMENT_ASSIGN,
                    .data.assign.assignee = expr,
                    .data.assign.value = value,
                    .data.assign.type = token_assign.type
                };
            }
        }
    }
}

void statement_free(Statement *statement) {
    switch (statement->type) {
        case STATEMENT_VAR_DECLARE:
            type_free(&statement->data.var_declare.type);
            if (statement->data.var_declare.has_assign) {
                expr_free(&statement->data.var_declare.assign);
            }
            break;
        case STATEMENT_INCREMENT:
            expr_free(&statement->data.increment);
            break;
        case STATEMENT_DEINCREMENT:
            expr_free(&statement->data.deincrement);
            break;
        case STATEMENT_ASSIGN:
            expr_free(&statement->data.assign.assignee);
            expr_free(&statement->data.assign.value);
            break;
        case STATEMENT_EXPR:
            expr_free(&statement->data.expr);
            break;

        case STATEMENT_LABEL:
        case STATEMENT_LABEL_GOTO:
            break;

        case STATEMENT_RETURN:
            expr_free(&statement->data.return_expr);
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
            break;
        case STATEMENT_INCREMENT:
            print("++"); // I don't like doing this, but idk how to do this better right now.
            expr_print(&statement->data.increment);
            break;
        case STATEMENT_DEINCREMENT:
            print("--");
            expr_print(&statement->data.deincrement);
            break;
        case STATEMENT_ASSIGN:
            expr_print(&statement->data.assign.assignee);
            printf(" %s ", string_assigns[statement->data.assign.type - TOKEN_ASSIGN_MIN]);
            expr_print(&statement->data.assign.value);
            break;
        case STATEMENT_EXPR:
            expr_print(&statement->data.expr);
            break;
        case STATEMENT_LABEL:
            printf("%s %s", string_keywords[TOKEN_KEYWORD_LABEL - TOKEN_KEYWORD_MIN], string_cache_get(statement->data.label));
            break;
        case STATEMENT_LABEL_GOTO:
            printf("%s %s", string_keywords[TOKEN_KEYWORD_GOTO - TOKEN_KEYWORD_MIN], string_cache_get(statement->data.label_goto));
            break;
        case STATEMENT_RETURN:
            printf("%s ", string_keywords[TOKEN_KEYWORD_RETURN - TOKEN_KEYWORD_MIN]);
            expr_print(&statement->data.return_expr);
            break;
    }  
}

Scope scope_parse(Lexer *lexer) {
    switch (lexer_token_peek(lexer).type) {
        case TOKEN_CURLY_BRACE_OPEN: {
            Token token_open = lexer_token_get(lexer);

            int scope_count = 0;
            Scope *scopes = NULL;
            
            while (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
                scope_count++;
                scopes = realloc(scopes, sizeof(Scope) * scope_count);
                scopes[scope_count - 1] = scope_parse(lexer);
            }

            Token token_close = lexer_token_get(lexer);
            
            return (Scope) {
                .location = location_expand(token_open.location, token_close.location),
                .type = SCOPE_BLOCK,
                .data.block.scope_count = scope_count,
                .data.block.scopes = scopes
            };
        }
       
        case TOKEN_KEYWORD_IF: {
            Token token_if = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            
            Location location;
            Scope *scope_if = malloc(sizeof(Scope));
            *scope_if = scope_parse(lexer);

            Scope *scope_else;
            if (lexer_token_peek(lexer).type == TOKEN_KEYWORD_ELSE) {
                lexer_token_get(lexer);
                scope_else = malloc(sizeof(Scope));
                *scope_else = scope_parse(lexer);
                location = location_expand(token_if.location, scope_else->location);
            } else {
                location = location_expand(token_if.location, scope_if->location);
                scope_else = NULL;
            }

            return (Scope) {
                .location = location,
                .type = SCOPE_CONDITIONAL,
                .data.conditional.condition = expr,
                .data.conditional.scope_if = scope_if,
                .data.conditional.scope_else = scope_else
            };
        }

        case TOKEN_KEYWORD_FOR: {
            Token token_for = lexer_token_get(lexer);
            
            if (lexer_token_peek_2(lexer).type == TOKEN_KEYWORD_IN) {
                Token token_id = lexer_token_peek(lexer);
                if (token_id.type != TOKEN_ID) {
                    error_exit(token_id.location, "Expected the name of the iteration variable in a for .. in loop.");
                }
                lexer_token_get(lexer);
                lexer_token_get(lexer);
                
                Expr array = expr_parse(lexer);
                Scope *scope = malloc(sizeof(Scope));
                *scope = scope_parse(lexer);

                return (Scope) {
                    .location = location_expand(token_for.location, scope->location),
                    .type = SCOPE_LOOP_FOR_EACH,
                    .data.loop_for_each.element = token_id.data.id,
                    .data.loop_for_each.array = array,
                    .data.loop_for_each.scope = scope
                };
            }

            Statement init = statement_parse(lexer);
            
            if (lexer_token_peek(lexer).type != TOKEN_SEMICOLON) error_exit(init.location, "Expected a semicolon after the initialization condition of a for loop.");
            lexer_token_get(lexer);
            
            Expr expr = expr_parse(lexer);

            if (lexer_token_peek(lexer).type != TOKEN_SEMICOLON) error_exit(expr.location, "Expected a semicolon after the condition in a for loop.");
            lexer_token_get(lexer);

            Statement step = statement_parse(lexer);
            
            Scope *scope = malloc(sizeof(Scope));
            *scope = scope_parse(lexer);
            
            return (Scope) {
                .location = location_expand(token_for.location, scope->location),
                .type = SCOPE_LOOP_FOR,
                .data.loop_for.init = init,
                .data.loop_for.expr = expr,
                .data.loop_for.step = step,
                .data.loop_for.scope = scope
            };
        }
        
        case TOKEN_KEYWORD_WHILE: {
            Token token_while = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            Scope *scope = malloc(sizeof(Scope));
            *scope = scope_parse(lexer);

            return (Scope) {
                .location = location_expand(token_while.location, scope->location),
                .type = SCOPE_LOOP_WHILE,
                .data.loop_while.expr = expr,
                .data.loop_while.scope = scope
            };
        }

        // not including the _ (defualt case) for now.
        case TOKEN_KEYWORD_MATCH: {
            Token token_match = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            Token open_brace_token = lexer_token_get(lexer);
            int case_count = 0;
            int case_count_allocated = 2; // defaults to 2 match statements

            if (open_brace_token.type != TOKEN_CURLY_BRACE_OPEN) { 
                error_exit(open_brace_token.location, "Expected an open curly brace after match expression.");
            }

            MatchCase *cases = malloc(sizeof(MatchCase) * case_count_allocated);
            while (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
                Token token_pipe = lexer_token_peek(lexer);
                if (token_pipe.type != TOKEN_OP_BITWISE_OR) {
                    error_exit(token_pipe.location, "Expected a '|' at the beginning of a match statement case.");
                }
                lexer_token_get(lexer);

                Token token_id = lexer_token_peek(lexer);
                if (token_id.type != TOKEN_ID) { 
                    error_exit(token_id.location, "Expected an identifier of a match case.");
                }
                lexer_token_get(lexer);

                Token token_lambda = lexer_token_peek(lexer);
                if (token_lambda.type != TOKEN_LAMBDA) { 
                    error_exit(token_lambda.location, "Expected a '->' between a match case and its body."); 
                }
                lexer_token_get(lexer);

                int scope_count = 0;
                int scope_count_allocated = 2;
                Scope *scopes = malloc(sizeof(Scope) * scope_count_allocated);

                while (lexer_token_peek(lexer).type != TOKEN_OP_BITWISE_OR && lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {                   
                    scope_count++;
                    if (scope_count > scope_count_allocated) {
                        scope_count_allocated *= 2; // double the size allocated for scopes
                        cases = realloc(cases, sizeof(Scope) * scope_count_allocated);
                    }
                    scopes[scope_count - 1] = scope_parse(lexer);
                }

                case_count++;
                if (case_count > case_count_allocated) {
                    case_count_allocated *= 2; // double the size for matches
                    cases = realloc(cases, sizeof(MatchCase) * case_count_allocated);
                }
                
                cases[case_count - 1] = (MatchCase) {
                    .location = location_expand(token_id.location, scopes[scope_count - 1].location),
                    .match_id = token_id.data.id,
                    .declares = false, // TODO: add parsing for a declared var in match
                    .scope_count = scope_count,
                    .scopes = scopes
                };
                // NOTE: is it worth realloc() to match_count to save space ? (same for scopes)
            }

            Token token_close = lexer_token_get(lexer);

            return (Scope) {
                .location = location_expand(token_match.location, token_close.location),
                .type = SCOPE_MATCH,
                .data.match.expr = expr,
                .data.match.case_count = case_count,
                .data.match.cases = cases,
            };
        }

        default: {
            Statement statement = statement_parse(lexer);
            Token token_semicolon = lexer_token_peek(lexer);
            if (token_semicolon.type != TOKEN_SEMICOLON) {
                error_exit(statement.location, "Expected a semicolon after a statement.");
            }
            lexer_token_get(lexer);

            return (Scope) {
                .location = location_expand(statement.location, token_semicolon.location),
                .type = SCOPE_STATEMENT,
                .data.statement = statement
            };
        }
    }
}

void scope_free(Scope *scope) {
    switch (scope->type) {
        case SCOPE_STATEMENT:
            statement_free(&scope->data.statement);
            break;
        case SCOPE_CONDITIONAL:
            expr_free(&scope->data.conditional.condition);
            scope_free(scope->data.conditional.scope_if);
            free(scope->data.conditional.scope_if);
            if (scope->data.conditional.scope_else) {
                scope_free(scope->data.conditional.scope_else);
                free(scope->data.conditional.scope_else);
            }
            break;
        case SCOPE_LOOP_FOR:
            statement_free(&scope->data.loop_for.init);
            expr_free(&scope->data.loop_for.expr);
            statement_free(&scope->data.loop_for.step);
            scope_free(scope->data.loop_for.scope);
            free(scope->data.loop_for.scope);
            break;
        case SCOPE_LOOP_FOR_EACH:
            expr_free(&scope->data.loop_for_each.array);
            scope_free(scope->data.loop_for_each.scope);
            free(scope->data.loop_for_each.scope);
            break;
        case SCOPE_LOOP_WHILE:
            expr_free(&scope->data.loop_while.expr);
            scope_free(scope->data.loop_while.scope);
            free(scope->data.loop_while.scope);
            break;
        case SCOPE_BLOCK:
            for (int i = 0; i < scope->data.block.scope_count; i++) scope_free(scope->data.block.scopes + i);
            free(scope->data.block.scopes);
            break;
        case SCOPE_MATCH:
            expr_free(&scope->data.match.expr);
            for (int i = 0; i < scope->data.match.case_count; ++i) {
                if (scope->data.match.cases[i].declares)
                    statement_free(&scope->data.match.cases[i].declared_var);
                int scope_count = scope->data.match.cases[i].scope_count;
                for (int j = 0; j < scope_count; ++j) 
                    scope_free(scope->data.match.cases[i].scopes + j);
                free(scope->data.match.cases[i].scopes);
            }
            free(scope->data.match.cases);
            break;
    }
}

void scope_print(Scope *scope, int indentation) {
    switch (scope->type) {
        case SCOPE_STATEMENT:
            statement_print(&scope->data.statement);
            putchar(TOKEN_SEMICOLON);
            putchar('\n');
            break;
            
        case SCOPE_CONDITIONAL:
            print(string_keywords[TOKEN_KEYWORD_IF - TOKEN_KEYWORD_MIN]);
            putchar(' ');
            expr_print(&scope->data.conditional.condition);
            putchar(' ');
            scope_print(scope->data.conditional.scope_if, indentation);
            if (scope->data.conditional.scope_else) {
                for (int i = 0; i < indentation; i++) print(STR_INDENTATION);
                print(string_keywords[TOKEN_KEYWORD_ELSE - TOKEN_KEYWORD_MIN]);
                putchar(' ');
                scope_print(scope->data.conditional.scope_else, indentation);
            }
            break;
        case SCOPE_LOOP_FOR:
            print(string_keywords[TOKEN_KEYWORD_FOR - TOKEN_KEYWORD_MIN]);
            putchar(' ');
            statement_print(&scope->data.loop_for.init);
            printf("%c ", TOKEN_SEMICOLON);
            expr_print(&scope->data.loop_for.expr);
            printf("%c ", TOKEN_SEMICOLON);
            statement_print(&scope->data.loop_for.step);
            putchar(' ');
            scope_print(scope->data.loop_for.scope, indentation);
            break;

        case SCOPE_LOOP_FOR_EACH:
            print(string_keywords[TOKEN_KEYWORD_FOR - TOKEN_KEYWORD_MIN]);
            printf(" %s %s ", string_cache_get(scope->data.loop_for_each.element), string_keywords[TOKEN_KEYWORD_IN - TOKEN_KEYWORD_MIN]);
            expr_print(&scope->data.loop_for_each.array);
            putchar(' ');
            scope_print(scope->data.loop_for_each.scope, indentation);
            break;
            
        case SCOPE_LOOP_WHILE:
            print(string_keywords[TOKEN_KEYWORD_WHILE - TOKEN_KEYWORD_MIN]);
            putchar(' ');
            expr_print(&scope->data.loop_while.expr);
            putchar(' ');
            scope_print(scope->data.loop_while.scope, indentation);
            break;

        case SCOPE_BLOCK:
            printf("%c\n", TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                for (int j = 0; j <= indentation; j++) print(STR_INDENTATION);
                scope_print(scope->data.block.scopes + i, indentation + 1);
            }
            for (int i = 0; i < indentation; i++) print(STR_INDENTATION);
            printf("%c\n", TOKEN_CURLY_BRACE_CLOSE);
            break;

        case SCOPE_MATCH:
            print(string_keywords[TOKEN_KEYWORD_MATCH - TOKEN_KEYWORD_MIN]);
            putchar(' ');
            expr_print(&scope->data.match.expr);
            printf(" %c\n", TOKEN_CURLY_BRACE_OPEN);
            
            for (int i = 0; i < scope->data.match.case_count; ++i) {
                for (int j = 0; j < indentation; ++j) print(STR_INDENTATION);
                
                printf("%s ", string_operators[TOKEN_OP_BITWISE_OR - TOKEN_OP_MIN]);
                print(string_cache_get(scope->data.match.cases[i].match_id));
                printf(" ->\n");
                for (int j = 0; j < scope->data.match.cases[i].scope_count; ++j) {
                    for (int j = 0; j <= indentation; ++j) {
                        print(STR_INDENTATION);
                    }
                    scope_print(scope->data.match.cases[i].scopes + j, indentation + 1);
                }
            }

            for (int i = 0; i < indentation; i++) print(STR_INDENTATION);
            printf("%c\n", TOKEN_CURLY_BRACE_CLOSE);
            break;
    }
}

Declaration declaration_parse(Lexer *lexer) {
    Token identifier = lexer_token_get(lexer);
    if (identifier.type != TOKEN_ID) {
        error_exit(identifier.location, "Expected a non-keyword identifier for the union member.");
    }
    StringId varname = identifier.data.id;
    Token keyword = lexer_token_get(lexer);

    switch (keyword.type) {

        case TOKEN_KEYWORD_ENUM: {
            if (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_OPEN) {
                error_exit(keyword.location, "Expected an opening curly brace starting an enum declaration.");
            }
            lexer_token_get(lexer);
            
            int member_count = 0;
            int member_count_alloc = 2;
            StringId *members = malloc(sizeof(StringId) * member_count_alloc);
            
            while (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
                Token token_id = lexer_token_get(lexer);
                if (token_id.type != TOKEN_ID) {
                    error_exit(token_id.location, "Expected an identifier as the value of an enum.");
                }
                member_count++;
                if (member_count > member_count_alloc) {
                    member_count_alloc *= 2;
                    members = realloc(members, sizeof(StringId) * member_count_alloc);
                }
                members[member_count - 1] = token_id.data.id;
                if (lexer_token_peek(lexer).type != TOKEN_SEMICOLON) {
                    error_exit(token_id.location, "Expected a semicolon after an enum member.");
                }
                lexer_token_get(lexer);
            }

            Token enum_token_end = lexer_token_get(lexer);
            return (Declaration) {
                .location = location_expand(identifier.location, enum_token_end.location),
                .type = DECLARATION_ENUM,
                .id = varname,
                .data.d_enum.member_count = member_count,
                .data.d_enum.members = members
            };
        }

        case TOKEN_KEYWORD_STRUCT:
        case TOKEN_KEYWORD_UNION: {
            int declaration_type;
            if (keyword.type == TOKEN_KEYWORD_STRUCT) declaration_type = DECLARATION_STRUCT;
            else if (keyword.type == TOKEN_KEYWORD_UNION) declaration_type = DECLARATION_UNION;
            else assert(false);

            if (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_OPEN) {
                error_exit(keyword.location, "Expected an opening curly brace when declaring a complex type.");
            }
            lexer_token_get(lexer);
            
            int member_count = 0;
            int member_count_alloc = 2;
            TypeMember *members = malloc(sizeof(TypeMember) * member_count_alloc);
            while (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
                Token token_id = lexer_token_get(lexer);
                if (token_id.type != TOKEN_ID) {
                    error_exit(token_id.location, "Expected the name of a type member here.");
                }
                if (lexer_token_get(lexer).type != TOKEN_COLON) {
                    error_exit(token_id.location, "Expected a colon after the name of a member in a complex type declaration.");
                }
                Type type = type_parse(lexer);
                if (lexer_token_get(lexer).type != TOKEN_SEMICOLON) {
                    error_exit(location_expand(token_id.location, type.location), "Expected a semicolon after a member in a complex type declaration."); 
                }
                member_count++;
                if (member_count > member_count_alloc) {
                    member_count_alloc *= 2;
                    members = realloc(members, sizeof(TypeMember) * member_count_alloc);
                }
                members[member_count - 1] = (TypeMember) {
                    .id = token_id.data.id,
                    .type = type
                };
            }
            Token token_end = lexer_token_get(lexer);
            return (Declaration) {
                .location = location_expand(identifier.location, token_end.location),
                .type = declaration_type,
                .id = varname,
                .data.d_complex_type.member_count = member_count,
                .data.d_complex_type.members = members
            };
        }
        
        case TOKEN_KEYWORD_SUM: {
            if (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_OPEN) {
                error_exit(keyword.location, "Expected an opening curly brace when declaring a sum type.");
            }
            lexer_token_get(lexer);
            
            int member_count = 0;
            int member_count_alloc = 2;
            SumMember *members = malloc(sizeof(SumMember) * member_count_alloc);

            while (lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
                Token sum_token_id = lexer_token_get(lexer);
                if (sum_token_id.type != TOKEN_ID) {
                    error_exit(sum_token_id.location, "Expected an identifier as the name of a sum member.");
                }

                SumMember member;
                member.id = sum_token_id.data.id;
                if (lexer_token_peek(lexer).type == TOKEN_COLON) {
                    lexer_token_get(lexer);
                    member.type_exists = true;
                    member.type = type_parse(lexer);
                } else member.type_exists = false;

                member_count++;
                if (member_count > member_count_alloc) {
                    member_count_alloc *= 2;
                    members = realloc(members, sizeof(SumMember) * member_count_alloc);
                }
                members[member_count - 1] = member;

                if (lexer_token_get(lexer).type != TOKEN_SEMICOLON) {
                    error_exit(sum_token_id.location, "Expected a semicolon after a sum member.");
                }
            }
            Token token_end = lexer_token_get(lexer);

            return (Declaration) {
                .location = location_expand(identifier.location, token_end.location),
                .id = varname,
                .type = DECLARATION_SUM,
                .data.d_sum.member_count = member_count,
                .data.d_sum.members = members
            };
        }

        case TOKEN_PAREN_OPEN: {
            FunctionParameter *parameters = NULL;
            int parameter_count = 0;

            if (lexer_token_peek(lexer).type != TOKEN_PAREN_CLOSE) {
                int parameter_count_allocated = 2;
                parameters = malloc(sizeof(FunctionParameter) * parameter_count_allocated);

                while (true) {
                    Token parameter_id = lexer_token_peek(lexer);
                    if (parameter_id.type != TOKEN_ID) {
                        error_exit(parameter_id.location, "Expected the name of a function parameter to be a non-keyword literal.");
                    }
                    lexer_token_get(lexer);
                    if (lexer_token_peek(lexer).type != TOKEN_COLON) {
                        error_exit(parameter_id.location, "Expected a colon after a function parameter name.");
                    }
                    lexer_token_get(lexer);
                    Type type = type_parse(lexer);
                    parameter_count++;
                    if (parameter_count > parameter_count_allocated) {
                        parameter_count_allocated *= 2;
                        parameters = realloc(parameters, sizeof(FunctionParameter) * parameter_count_allocated);
                    }
                    parameters[parameter_count - 1] = (FunctionParameter) {
                        .id = parameter_id.data.id,
                        .type = type
                    };

                    if (lexer_token_peek(lexer).type == TOKEN_COMMA) {
                        lexer_token_get(lexer); 
                        continue;
                    }

                    if (lexer_token_peek(lexer).type == TOKEN_PAREN_CLOSE) break;
                }
            }

            lexer_token_get(lexer);
            Type return_type = type_parse(lexer);
            Scope scope = scope_parse(lexer);

            return (Declaration) {
                .type = DECLARATION_FUNCTION,
                .id = varname,
                .data.d_function.parameters = parameters,
                .data.d_function.parameter_count = parameter_count,
                .data.d_function.return_type = return_type,
                .data.d_function.scope = scope
            };
        }

        default: 
            error_exit(keyword.location, "Expected a literal or function declaration here.");
            return (Declaration) {{ 0 }}; // have to put extra braces to work around compiler bug on version of gcc used by cslab
    }
}

void declaration_free(Declaration *declaration) {
    switch (declaration->type) {
        case DECLARATION_STRUCT:
        case DECLARATION_UNION:
            for (int i = 0; i < declaration->data.d_complex_type.member_count; i++) {
                type_free(&declaration->data.d_complex_type.members[i].type);
            }
            free(declaration->data.d_complex_type.members);
            break;

            
        case DECLARATION_SUM:
            for (int i = 0; i < declaration->data.d_sum.member_count; i++) {
                SumMember member = declaration->data.d_sum.members[i];
                if (member.type_exists) type_free(&member.type);
            }
            free(declaration->data.d_sum.members);
            break;

        case DECLARATION_ENUM:
            free(declaration->data.d_enum.members);
            break;

        case DECLARATION_FUNCTION:
            for (int i = 0; i < declaration->data.d_function.parameter_count; i++) {
                type_free(&declaration->data.d_function.parameters[i].type);
            }
            free(declaration->data.d_function.parameters);
            type_free(&declaration->data.d_function.return_type);
            scope_free(&declaration->data.d_function.scope);
            break;
    }
}

void declaration_print(Declaration *declaration) {
    printf("%s ", string_cache_get(declaration->id));
    switch (declaration->type) {
        case DECLARATION_ENUM:
            printf("%s %c\n", string_keywords[TOKEN_KEYWORD_ENUM - TOKEN_KEYWORD_MIN], TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < declaration->data.d_enum.member_count; i++) {
                printf("%s%s%c\n", STR_INDENTATION, string_cache_get(declaration->data.d_enum.members[i]), TOKEN_SEMICOLON);
            }
            printf("%c\n", TOKEN_CURLY_BRACE_CLOSE);
            break;
        
        case DECLARATION_STRUCT:
            print(string_keywords[TOKEN_KEYWORD_STRUCT - TOKEN_KEYWORD_MIN]);
            goto complex_type_print;
        case DECLARATION_UNION:
            print(string_keywords[TOKEN_KEYWORD_UNION - TOKEN_KEYWORD_MIN]);
            complex_type_print:
            printf(" %c\n", TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < declaration->data.d_complex_type.member_count; i++) {
                printf("%s%s%c ", STR_INDENTATION, string_cache_get(declaration->data.d_complex_type.members[i].id), TOKEN_COLON);
                type_print(&declaration->data.d_complex_type.members[i].type);
                printf("%c\n", TOKEN_SEMICOLON);
            }
            printf("%c\n", TOKEN_CURLY_BRACE_CLOSE);    
            break;

        case DECLARATION_SUM:
            printf("%s %c\n", string_keywords[TOKEN_KEYWORD_SUM - TOKEN_KEYWORD_MIN], TOKEN_CURLY_BRACE_OPEN);
            for (int i = 0; i < declaration->data.d_sum.member_count; i++) {
                SumMember member = declaration->data.d_sum.members[i];
                printf("%s%s", STR_INDENTATION, string_cache_get(member.id));
                if (member.type_exists) {
                    printf("%c ", TOKEN_COLON);
                    type_print(&member.type);
                }
                printf("%c\n", TOKEN_SEMICOLON);
            }
            printf("%c\n", TOKEN_CURLY_BRACE_CLOSE);
            break;

        case DECLARATION_FUNCTION:
            putchar(TOKEN_PAREN_OPEN);
            if (declaration->data.d_function.parameter_count > 0) {
                for (int i = 0; i < declaration->data.d_function.parameter_count - 1; i++) {
                    printf("%s%c ", string_cache_get(declaration->data.d_function.parameters[i].id), TOKEN_COLON);
                    type_print(&declaration->data.d_function.parameters[i].type);
                    putchar(TOKEN_COMMA);
                }
                printf("%s%c ", string_cache_get(declaration->data.d_function.parameters[declaration->data.d_function.parameter_count - 1].id), TOKEN_COLON);
                type_print(&declaration->data.d_function.parameters[declaration->data.d_function.parameter_count - 1].type);
            }
            printf("%c ", TOKEN_PAREN_CLOSE);
            type_print(&declaration->data.d_function.return_type);
            putchar(' ');
            scope_print(&declaration->data.d_function.scope, 0);
            break;
    }
}
