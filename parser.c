#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "print.h"
#include "token.h"

static Expr expr_parse_precedence(Lexer *lexer, int precedence) {
    Expr expr;
    TokenType peek = lexer_token_peek(lexer)->type;
    if (peek == TOKEN_LITERAL) {
        Token token = lexer_token_get(lexer);
        expr = (Expr) {
            .line_start = token.line_idx,
            .line_end = token.line_idx,
            .char_start = token.char_idx,
            .char_end = token.char_idx + token.len,
            .type = EXPR_LITERAL,
            .data.literal = token.data.literal
        };
        // TODO: what to do about freeing token? Don't technically need to free it, but it would be nice to.
        // Cannot free it because the literal_string uses the literal from the token.
        
    } else if (peek == TOKEN_PAREN_OPEN) { 
        Token token_open = lexer_token_get(lexer);
        expr.line_start = token_open.line_idx;
        expr.char_start = token_open.char_idx;
        token_free(&token_open);
        
        Expr operand = expr_parse(lexer);
        if (EXPR_ERROR_MIN <= operand.type && operand.type <= EXPR_ERROR_MAX) {
            return operand;
        }
        
        if (lexer_token_peek(lexer)->type != TOKEN_PAREN_CLOSE) {
            expr.line_end = operand.line_end;
            expr.char_end = operand.char_end;
            expr.type = EXPR_ERROR_EXPECTED_PAREN_CLOSE;
            expr_free(&operand);
            return expr;
        }        

        Token token_close = lexer_token_get(lexer);
        Expr *operand_ptr = malloc(sizeof(Expr));
        *operand_ptr = operand;
        
        expr = (Expr) {
            .line_end = token_close.line_idx,
            .char_end = token_close.char_idx + token_close.len,
            .type = EXPR_UNARY,
            .data.unary.operator = EXPR_UNARY_PAREN,
            .data.unary.operand = operand_ptr
        };
        
        token_free(&token_close);
    } else {
        // TODO: Make some kind of dummy operator, this causes UNDEFINED BEHAVIOR
        // lexer_error_push(lexer, lexer_token_peek(lexer), LEXER_ERROR_MISSING_EXPR);
    }
    
    while (true) {     
        TokenType op_type = lexer_token_peek(lexer)->type;
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
            // line start and char start remain the same so no need to change.
            .line_end = rhs.line_end,
            .char_end = rhs.char_end,
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
        
        case EXPR_LITERAL:
            literal_print(&expr->data.literal);
            break;
        
        default:
            printf("[Unrecognized expr type %i]", expr->type);
            break;
    }
}
