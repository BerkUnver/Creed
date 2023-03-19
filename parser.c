#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "print.h"
#include "token.h"

void parser_error_print(ParserError error, Location location) {
    location_print(location);
    print(": ");
    switch (error) {
        case PARSER_ERROR_NO_TYPE_ID:
            print("Expected a type id at the beginning of a type signature.");
            break;
        case PARSER_ERROR_NO_ARRAY_CLOSING_BRACKET:
            print("Expected a closing bracket to match the opening bracket of an array type signature.");
            break;
        case PARSER_ERROR_NO_PAREN_CLOSE:
            print("Expected a closing parenthesis to match the opening parenthesis of a parenthesied expression.");
            break;
        case PARSER_ERROR_NO_FUNCTION_PARAM_COMMA:
            print("Expected a comma separating consecutive arguments of a function.");
            break;
        case PARSER_ERROR_NO_EXPR:
            print("Expected an expression here.");
            break;
        default:
            assert(false);
            return;
    }
}

Type type_parse(Lexer *lexer) {
    if (lexer_token_peek(lexer)->type != TOKEN_ID) {
        Token token_err = lexer_token_get(lexer);
        Type type_err = {
            .location = token_err.location,
            .type = TYPE_ERROR,
            .data.error = PARSER_ERROR_NO_TYPE_ID
        };
        token_free(&token_err);
        return type_err;
    }
    
    Token token_id = lexer_token_get(lexer); // TODO: a hack. Don't need to free as id ownership is transferred
    Type type = {
        .location = token_id.location,
        .type = TYPE_ID,
        .data.id = token_id.data.id
    };

    while (true) {
        int ptr_type; // I don't wanna make the type type a named type just for this
        
        switch (lexer_token_peek(lexer)->type) {
            case TOKEN_OP_MULTIPLY:
                ptr_type = TYPE_PTR;
                break;

            case TOKEN_QUESTION_MARK:
                ptr_type = TYPE_PTR_NULLABLE;
                break;
            
            case TOKEN_BRACKET_OPEN: {
                Token token_open = lexer_token_get(lexer);
                token_free(&token_open);
                
                if (lexer_token_peek(lexer)->type != TOKEN_BRACKET_CLOSE) {
                    Token token_error = lexer_token_get(lexer);
                    type_free(&type);
                    Type type_err = {
                        .location = token_error.location,
                        .type = TYPE_ERROR,
                        .data.error = PARSER_ERROR_NO_ARRAY_CLOSING_BRACKET,
                    };
                    token_free(&token_error);
                    return type_err;
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
        token_free(&token_end);
    }
}

void type_free(Type *type) {
    switch (type->type) {
        case TYPE_ID:
            free(type->data.id);
            break;
        
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
            print(type->data.id);
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
       
        case TYPE_ERROR:
            print("Type error: ");
            parser_error_print(type->data.error, type->location);
            break;

        default: break;
    }
}

static Expr expr_parse_precedence(Lexer *lexer, int precedence) {
    Token *peek = lexer_token_peek(lexer);

    Expr expr;
    switch (peek->type) {
        case TOKEN_LITERAL: {
            Token token = lexer_token_get(lexer);
            expr.type = EXPR_LITERAL;
            expr.location = token.location;
            expr.data.literal = token.data.literal;
            // TODO: a hack. Don't need to free token as literal ownership is transferred.
        } break;
      
        case TOKEN_PAREN_OPEN: {
            Token token_open = lexer_token_get(lexer);
            token_free(&token_open);
            
            Expr operand = expr_parse(lexer);
            if (operand.type == EXPR_ERROR) return operand;
            
            if (lexer_token_peek(lexer)->type != TOKEN_PAREN_CLOSE) {
                expr = (Expr) {
                    .location = location_expand(token_open.location, operand.location),
                    .type = EXPR_ERROR,
                    .data.error = PARSER_ERROR_NO_PAREN_CLOSE
                };
                expr_free(&operand);
                return expr;
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
            
            token_free(&token_close);
        } break;

        case TOKEN_ID: {
            Token token_id = lexer_token_get(lexer);
            expr.type = EXPR_ID;
            expr.data.id = token_id.data.id;
            expr.location = token_id.location;
        } break;
        
        default: {
            Token token_err = lexer_token_get(lexer);
            expr.location = token_err.location;
            expr.type = EXPR_ERROR;
            expr.data.error = PARSER_ERROR_NO_EXPR;
            token_free(&token_err);
            return expr;
        }
    }
    
    while (true) {     
        TokenType op_type = lexer_token_peek(lexer)->type;
        if (op_type == TOKEN_KEYWORD_TYPECAST) {
            Token token_typecast = lexer_token_get(lexer);
            token_free(&token_typecast);
            
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
            Token paren_open = lexer_token_get(lexer);
            token_free(&paren_open);
            
            int param_count = 0;
            Expr *params = NULL;
            
            if (lexer_token_peek(lexer)->type != TOKEN_PAREN_CLOSE) {
                while (true) {
                    Expr param = expr_parse(lexer);
                    if (param.type == EXPR_ERROR) { 
                        for (int i = 0; i < param_count; i++) expr_free(&params[i]);
                        free(params);
                        expr_free(&expr);
                        return param;
                    }
                    
                    param_count++;
                    params = realloc(params, sizeof(Expr) * param_count);
                    params[param_count - 1] = param;
                    
                    if (lexer_token_peek(lexer)->type == TOKEN_PAREN_CLOSE) break;
                    if (lexer_token_peek(lexer)->type == TOKEN_COMMA) {
                        lexer_token_get(lexer);
                    } else {
                        Token token_end = lexer_token_get(lexer);
                        for (int i = 0; i < param_count; i++) expr_free(&params[i]);
                        free(params);
                        expr_free(&expr);
                        token_free(&token_end);
                        return (Expr) {
                            .location = token_end.location,
                            .type = EXPR_ERROR,
                            .data.error = PARSER_ERROR_NO_FUNCTION_PARAM_COMMA
                        };
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
            token_free(&paren_close);
            continue;
        }

        if (op_type < TOKEN_OP_MIN || TOKEN_OP_MAX < op_type) return expr;
        
        int op_precedence = operator_precedences[op_type - TOKEN_OP_MIN];
        if (op_precedence < precedence) return expr; 
        
        Token op = lexer_token_get(lexer);
        token_free(&op);

        Expr rhs = expr_parse_precedence(lexer, op_precedence);
        if (rhs.type == EXPR_ERROR) {
            expr_free(&expr);
            return rhs;
        }
        
        Expr *lhs_ptr = malloc(sizeof(Expr));
        *lhs_ptr = expr;
        
        Expr *rhs_ptr = malloc(sizeof(Expr));
        *rhs_ptr = rhs;
        
        expr = (Expr) {
            .location = location_expand(expr.location, rhs.location), 
            .type = EXPR_BINARY,
            .data.binary.operator = op_type,
            .data.binary.lhs = lhs_ptr,
            .data.binary.rhs = rhs_ptr,
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
            for (int i = 0; i < expr->data.function_call.param_count - 1; i++) {
                expr_free(&expr->data.function_call.params[i]);
            }
            free(expr->data.function_call.params);
            expr_free(expr->data.function_call.function);
            free(expr->data.function_call.function);
            break;

        case EXPR_ID:
            free(expr->data.id);
            break;

        case EXPR_LITERAL:
            literal_free(&expr->data.literal);
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
            print(expr->data.id);
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

        case EXPR_ERROR:
            print("[Expression error: ");
            parser_error_print(expr->data.error, expr->location);
            putchar(']');
            break;

        default:
            assert(false);
            break;
    }
}
