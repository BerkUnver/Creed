#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"
#include "token.h"

typedef struct Expr {
    int line_start;
    int line_end; // expr can span multiple lines
    int char_start;
    int char_end;
    
    enum {
        EXPR_UNARY, 
        EXPR_BINARY,
        EXPR_FUNCTION_CALL,
        
        EXPR_LITERAL,
        
        EXPR_ERROR_MIN,
        EXPR_ERROR_NONE = EXPR_ERROR_MIN,
        EXPR_ERROR_EXPECTED_PAREN_CLOSE,
        EXPR_ERROR_EXPR_MISSING,
        EXPR_ERROR_MAX = EXPR_ERROR_EXPR_MISSING
    } type;

    union {
        struct {
            enum {
                EXPR_UNARY_NEGATE,
                EXPR_UNARY_NOT,
                EXPR_UNARY_REF,
                EXPR_UNARY_DEREF,
                EXPR_UNARY_PAREN
            } operator; 
            
            struct Expr *operand;
        } unary;

        struct {
            TokenType operator; // decided to use the token type directly for this to avoid code repetition. Only valid for binary operators.
            struct Expr *lhs; // lhs means left-hand side
            struct Expr *rhs; // rhs means right-hand side
        } binary;

        struct {
            char *id;
            struct Expr *params;
            int param_count;
        } function_call;

        Literal literal;
    } data;
} Expr;

Expr expr_parse(Lexer *lexer);
void expr_free(Expr *expr);
void expr_print(Expr *expr);

#endif
