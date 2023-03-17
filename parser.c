#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "print.h"
#include "token.h"

Type type_parse(Lexer *lexer) {
    if (lexer_token_peek(lexer)->type != TOKEN_ID) {
        // error handling
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
            case TOKEN_CARET:
                ptr_type = TYPE_PTR;
                break;

            case TOKEN_QUESTION_MARK:
                ptr_type = TYPE_PTR_NULLABLE;
                break;
            
            case TOKEN_BRACKET_OPEN: {
                Token token_open = lexer_token_get(lexer);
                token_free(&token_open);
                
                if (lexer_token_peek(lexer)->type != TOKEN_BRACKET_CLOSE) {
                    // error handling
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

void type_print(Type *type) {
    switch (type->type) {
        case TYPE_ID:
            print(type->data.id);
            break;
        case TYPE_PTR:
            type_print(type->data.sub_type);
            putchar(TOKEN_CARET);
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
    Token *peek = lexer_token_peek(lexer);

    Expr expr;
    expr.location = peek->location;

    switch(peek->type) {
        case TOKEN_LITERAL: {
            Token token = lexer_token_get(lexer);
            expr.type = EXPR_LITERAL;
            expr.data.literal = token.data.literal;
            // TODO: a hack. Don't need to free token as literal ownership is transferred.
        } break;
      
        case TOKEN_PAREN_OPEN: {
            Token token_open = lexer_token_get(lexer);
            token_free(&token_open);
            
            Expr operand = expr_parse(lexer);
            if (EXPR_ERROR_MIN <= operand.type && operand.type <= EXPR_ERROR_MAX) {
                return operand;
            }
            
            if (lexer_token_peek(lexer)->type != TOKEN_PAREN_CLOSE) {
                expr.location = location_expand(expr.location, operand.location);
                expr.type = EXPR_ERROR_EXPECTED_PAREN_CLOSE;
                expr_free(&operand);
                return expr;
            }        

            Token token_close = lexer_token_get(lexer);
            Expr *operand_ptr = malloc(sizeof(Expr));
            *operand_ptr = operand;
            
            expr = (Expr) {
                .location = location_expand(expr.location, operand.location),
                .type = EXPR_UNARY,
                .data.unary.operator = EXPR_UNARY_PAREN,
                .data.unary.operand = operand_ptr
            };
            
            token_free(&token_close);
        } break;

        case TOKEN_ID: {
            Token token_id = lexer_token_get(lexer);
            expr = (Expr) {
                .location = location_expand(expr.location, token_id.location),
                .type = EXPR_ID,
                .data.id = token_id.data.id // TODO: a hack, don't free tokene because the string is transferred here.
            };
        } break;
        
        default: break;// TODO: error handling
    }
    
    while (true) {     
        TokenType op_type = lexer_token_peek(lexer)->type;
        if (op_type == TOKEN_KEYWORD_TYPECAST) {
            Token token_as = lexer_token_get(lexer);
            token_free(&token_as);

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
        }

        if (op_type < TOKEN_OP_MIN || TOKEN_OP_MAX < op_type) return expr;
        
        int op_precedence = operator_precedences[op_type - TOKEN_OP_MIN];
        if (op_precedence < precedence) return expr; 
        
        Token op = lexer_token_get(lexer);
        token_free(&op);

        Expr rhs = expr_parse_precedence(lexer, op_precedence);
        if (EXPR_ERROR_MIN <= rhs.type && rhs.type <= EXPR_ERROR_MAX) {
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
        
        default:
            printf("[Unrecognized expr type %i]", expr->type);
            break;
    }
}
