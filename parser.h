#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"

typedef struct AstExpr {
    int line_start;
    int line_end; // expr can span multiple lines
    int char_start;
    int char_end;
    
    bool parenthesized;

    enum {
        EXPR_UNARY, 
        EXPR_BINARY,
        EXPR_FUNCTION_CALL,
        EXPR_LITERAL_INT
    } type;

    union {
        struct {
            enum {
                EXPR_UNARY_NEGATE,
                EXPR_UNARY_NOT,
                EXPR_UNARY_REFERENCE,
                EXPR_UNARY_DEREFERENCE,
                EXPR_UNARY_PAREN
            } operator;
        
            struct AstExpr *sub_expr;
        } unary;

        struct {
            enum {            
                EXPR_BINARY_ADD,
                EXPR_BINARY_SUBTRACT,
                EXPR_BINARY_MULTIPLY,
                EXPR_BINARY_DIVIDE,
                EXPR_BINARY_MODULO,
                
                EXPR_BINARY_EQUALS,
                EXPR_BINARY_NOT_EQUALS,
                
                EXPR_BINARY_GT,
                EXPR_BINARY_LT,
                EXPR_BINARY_GE,
                EXPR_BINARY_LE,

                EXPR_BINARY_LOGICAL_AND,
                EXPR_BINARY_LOGICAL_OR,
            } operator;

            struct AstExpr *lhs;
            struct AstExpr *rhs;
        } binary;

        struct {
            char *id;
            struct AstExpr *params;
            int param_count;
        } function_call;

        int literal_int;

    } data;
} AstExpr;

bool parse_expr(Lexer *lexer, AstExpr *expr);
void expr_free(AstExpr *expr);
#endif
