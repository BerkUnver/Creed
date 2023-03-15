#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

static Expr expr_parse_precedence(Lexer *lexer, int precedence) {
    TokenType peek = lexer_token_peek_type(lexer);
    Expr expr;
    if (TOKEN_LITERAL_MIN <= peek && peek <= TOKEN_LITERAL_MAX) {
        Token token = lexer_token_get(lexer);
        expr = (Expr) {
            .line_start = token.line_idx,
            .line_end = token.line_idx,
            .char_start = token.char_idx,
            .char_end = token.char_idx + token.len,
        };
        switch (token.type) { // think of a good way to eliminate this boilerplate
            case TOKEN_LITERAL_INT:
                expr.type = EXPR_LITERAL_INT;
                expr.data.literal_int = token.data.literal_int;
                break;
            case TOKEN_LITERAL_CHAR:
                expr.type = EXPR_LITERAL_CHAR;
                expr.data.literal_char = token.data.literal_char;
                break;
            case TOKEN_LITERAL_STRING:
                expr.type = EXPR_LITERAL_STRING;
                expr.data.literal_string = token.data.literal_string;
                break;
            case TOKEN_LITERAL_DOUBLE:
                expr.type = EXPR_LITERAL_DOUBLE;
                expr.data.literal_double = token.data.literal_double;
                break;
            default: // should never happen.
                assert(false);
                break;
        }
        // what to do about freeing token? Don't technically need to free it, but it would be nice to.
        // Cannot free it because the literal_string uses the literal from the token.
        
    } else if (peek == TOKEN_PAREN_OPEN) { 
        Token token_open = lexer_token_get(lexer);
        int line_start = token_open.line_idx;
        int char_start = token_open.char_idx;
        token_free(&token_open);
        
        Expr *operand = malloc(sizeof(Expr));
        *operand = expr_parse(lexer);
        
        int line_end;
        int char_end;
        if (lexer_token_peek_type(lexer) == TOKEN_PAREN_CLOSE) {
            Token token_close = lexer_token_get(lexer);
            line_end = token_close.line_idx;
            char_end = token_close.char_idx + token_close.len;
            token_free(&token_close);
        } else {
            line_end = operand->line_end;
            char_end = operand->char_end; 
            lexer_error_push(lexer, lexer_token_peek_len(lexer), LEXER_ERROR_EXPECTED_PAREN_CLOSE);
        }
                    
        expr = (Expr) {
            .line_start = line_start,
            .char_start = char_start,
            .line_end = line_end,
            .char_end = char_end,
            .type = EXPR_UNARY,
            .data.unary.operator = EXPR_UNARY_PAREN,
            .data.unary.operand = operand
        };
    } else {
        // TODO: Make some kind of dummy operator, this causes UNDEFINED BEHAVIOR
        lexer_error_push(lexer, lexer_token_peek_len(lexer), LEXER_ERROR_MISSING_EXPR);
    }
    
    while (true) {     
        TokenType op_type = lexer_token_peek_type(lexer); 
        if (op_type < TOKEN_OP_MIN || TOKEN_OP_MAX < op_type) return expr;
        int op_precedence = operator_precedences[op_type - TOKEN_OP_MIN];
        if (op_precedence < precedence) return expr; 
        Token op = lexer_token_get(lexer);
        token_free(&op);
        
        Expr *lhs = malloc(sizeof(Expr));
        *lhs = expr;
        
        Expr *rhs = malloc(sizeof(Expr));
        *rhs = expr_parse_precedence(lexer, op_precedence);
        
        expr = (Expr) {
            // line start and char start remain the same so no need to change.
            .line_end = rhs->line_end,
            .char_end = rhs->char_end,
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
        
        case EXPR_LITERAL_INT:
            printf("%i", expr->data.literal_int);
            break;
        
        default:
            printf("[Unrecognized expr type %i]", expr->type);
            break;
    }
}
