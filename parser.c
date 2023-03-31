#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "print.h"
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

            expr.type = EXPR_MEMBER_ACCESS;
            expr.data.member_access.operand = operand;
            expr.data.member_access.member = token_id.data.id;
            expr.location = location_expand(expr.location, token_id.location);
        
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
        
        case EXPR_MEMBER_ACCESS:
            expr_free(expr->data.member_access.operand);
            free(expr->data.member_access.operand);
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
        
        case EXPR_MEMBER_ACCESS:
            putchar('(');
            expr_print(expr->data.member_access.operand);
            putchar(')');
            putchar(TOKEN_DOT);
            print(string_cache_get(expr->data.member_access.member));
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
    Token token_var = lexer_token_peek(lexer);
    if (token_var.type != TOKEN_ID) {
        error_exit(token_var.location, "Expected a variable name at the start of a statement.");    
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

    statement.location = location_expand(token_var.location, type.location);
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
        
        case TOKEN_KEYWORD_FOR: {
            Token token_for = lexer_token_get(lexer);
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

        // not including the _ (defualt case) for now.
        case TOKEN_KEYWORD_MATCH: {
            Token token_match = lexer_token_get(lexer);
            Expr expr = expr_parse(lexer);
            Token open_brace_token = lexer_token_get(lexer);
            int case_count = 0;
            int allocated_case_count = 2; // defaults to 2 match statements

            if (open_brace_token.type != TOKEN_CURLY_BRACE_OPEN) { error_exit(open_brace_token.location, "Expected an open curly brace after match expression."); }

            MatchCase *cases = malloc(sizeof(MatchCase) * allocated_case_count);
            while(lexer_token_peek(lexer).type == TOKEN_OP_BITWISE_OR) {
                lexer_token_get(lexer);
                if (case_count >= allocated_case_count) {
                    allocated_case_count *= 2; // double the size for matches
                    cases = realloc(cases, sizeof(MatchCase) * allocated_case_count);
                }

                Token id_token = lexer_token_get(lexer);
                if (id_token.type != TOKEN_ID) { error_exit(id_token.location, "Expected an identifier."); }

                Token arrow_token = lexer_token_get(lexer);
                if (arrow_token.type != TOKEN_OP_RIGHT_ARROW) { error_exit(arrow_token.location, "Expected a '->' token"); }

                int statement_count = 0;
                int allocated_statement_count = 2;
                Statement *statements = malloc(sizeof(Statement) * allocated_statement_count);

                while(lexer_token_peek(lexer).type != TOKEN_OP_BITWISE_OR && lexer_token_peek(lexer).type != TOKEN_CURLY_BRACE_CLOSE) {
                    if (statement_count >= allocated_statement_count) {
                        allocated_statement_count *= 2; // double the size allocated for statements
                        cases = realloc(cases, sizeof(Statement) * allocated_statement_count);
                    }
                    statements[statement_count] = statement_parse(lexer);
                    statement_count += 1;
                    // consume semicolon:
                    Token end_statement_token = lexer_token_get(lexer);
                    if (end_statement_token.type != TOKEN_SEMICOLON) { error_exit(end_statement_token.location, "Expected a semicolon at end of statement."); }
                }
                cases[case_count] = (MatchCase) {
                    .location = location_expand(id_token.location, statements[statement_count].location),
                    .match_id = id_token.data.id,
                    .declares = false, // TODO: add parsing for a declared var in match
                    .statement_count = statement_count,
                    .statements = statements
                };
                case_count += 1;
                // NOTE: is it worth realloc() to match_count to save space ? (same for statements)
            }

            Token close_brace = lexer_token_get(lexer);
            if (close_brace.type != TOKEN_CURLY_BRACE_CLOSE) { error_exit(close_brace.location, "Expected a closing curly brace."); }

            return (Scope) {
                .location = location_expand(token_match.location, cases[case_count].location),
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
        case SCOPE_LOOP_FOR:
            statement_free(&scope->data.loop_for.init);
            expr_free(&scope->data.loop_for.expr);
            statement_free(&scope->data.loop_for.step);
            scope_free(scope->data.loop_for.scope);
            free(scope->data.loop_for.scope);
            break;
        case SCOPE_BLOCK:
            for (int i = 0; i < scope->data.block.scope_count; i++) scope_free(scope->data.block.scopes + i);
            free(scope->data.block.scopes);
            break;
        case SCOPE_MATCH:
            expr_free(&scope->data.match.expr);
            for (int i=0; i < scope->data.match.case_count; ++i) {
                if (scope->data.match.cases[i].declares)
                    statement_free(&scope->data.match.cases[i].declared_var);
                int stmt_count = scope->data.match.cases[i].statement_count;
                for (int j=0; j < stmt_count; ++j) statement_free(scope->data.match.cases[i].statements + j);
            }
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
            putchar(' ');
            putchar('{');
            putchar('\n');
            for(int i=0; i<scope->data.match.case_count; ++i) {
                for(int j=0; j<indentation; ++j) {putchar('\t');}
                putchar('|');
                print(string_cache_get((scope->data.match.cases + i)->match_id));
                printf("->");
                for(int j=0; j < (scope->data.match.cases + i)->statement_count; ++j) {
                    putchar('\n');
                    for(int j=0; j<indentation+1; ++j) {putchar('\t');}
                    statement_print(&(scope->data.match.cases + i)->statements[j]);
                }
                putchar('\n');
            }

            break;
    }
}



Constant constant_parse(Lexer *lexer) {
    Token constant_name = lexer_token_get(lexer);
    if (constant_name.type != TOKEN_ID) {
        error_exit(constant_name.location, "Expected the name of a constant in a top-level declaration.");
    }
    Token paren_open = lexer_token_get(lexer);
    if (paren_open.type != TOKEN_PAREN_OPEN) {
        error_exit(paren_open.location, "Expected the opening parenthesis of a function declaration list.");
    }

    while (true) {
        // parse parameters
    }

    // parse scope
}

void constant_free(Constant *constant) {
    switch (constant->type) {
        case CONSTANT_FUNCTION:
            for (int i = 0; i < constant->data.function.parameter_count; i++) {
                type_free(&constant->data.function.parameters[i].type);
            }
            free(constant->data.function.parameters);
            scope_free(&constant->data.function.scope);
            break;
    }
}
