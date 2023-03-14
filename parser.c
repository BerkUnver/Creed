#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

bool parse_expr(Lexer *lexer, Expr *expr) {
    bool success = false;
    switch (lexer_token_peek_type(lexer))
    {
        case TOKEN_LITERAL_INT: {
            Token token_start = lexer_token_get(lexer);
           
            Expr lhs = {
                .line_start = token_start.line_idx,
                .line_end = token_start.line_idx,
                .char_start = token_start.char_idx,
                .char_end = token_start.char_idx + token_start.len,
                .type = EXPR_LITERAL_INT,
                .data.literal_int = token_start.data.literal_int
            };

            token_free(&token_start);
            TokenType op_type = lexer_token_peek_type(lexer);
            if (op_type < TOKEN_OP_MIN || TOKEN_OP_MAX < op_type) { 
                *expr = lhs;
                success = true;
                break;
            }
           
            Token op = lexer_token_get(lexer);
            token_free(&op);
            
            Expr rhs;
            if (!parse_expr(lexer, &rhs)) {
                expr_free(&lhs);
                success = false;
                break;
            }

            Expr *ptr_lhs = malloc(sizeof(Expr));
            *ptr_lhs = lhs;
            Expr *ptr_rhs = malloc(sizeof(Expr));
            *ptr_rhs = rhs;

            *expr = (Expr) {
                .line_start = lhs.line_start,
                .line_end = rhs.line_end,
                .char_start = lhs.char_start,
                .char_end = rhs.char_end,
                .type = EXPR_BINARY,
                .data.binary.operator = op_type, // this is ok because operators correspond 1:1
                .data.binary.lhs = ptr_lhs,
                .data.binary.rhs = ptr_rhs
            };
            success = true;
        } break;

        case TOKEN_PAREN_OPEN: {
            Token token_open = lexer_token_get(lexer);
            Expr operand;
            if (!parse_expr(lexer, &operand)) {
                success = false;
            } else if (lexer_token_peek_type(lexer) != TOKEN_PAREN_CLOSE) {
                expr_free(expr);
                success = false;
                // push error
            } else {
                Token token_close = lexer_token_get(lexer);
                Expr *operand_ptr = malloc(sizeof(Expr));
                *operand_ptr = operand;
                *expr = (Expr) {
                    .line_start = token_open.line_idx,
                    .line_end = token_close.line_idx,
                    .char_start = token_open.char_idx,
                    .char_end =  token_close.line_idx + token_close.len,
                    .type = EXPR_UNARY,
                    .data.unary.operator = OP_UNARY_PAREN,
                    .data.unary.operand = operand_ptr
                };
                token_free(&token_close);
                success = true;
            }
            token_free(&token_open);
            break;
        }

        default: 
            success = false; 
            break;
    }
    return success;
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
            expr_print(expr->data.binary.lhs);
            printf(" %s ", string_operators[expr->data.binary.operator - OP_BINARY_ADD]);
            expr_print(expr->data.binary.rhs);
            break;
        
        case EXPR_LITERAL_INT:
            printf("%i", expr->data.literal_int);
            break;
        
        default:
            printf("[Unrecognized expr type %i]", expr->type);
            break;
    }
}
