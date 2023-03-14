#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"

typedef struct Expr {
    int line_start;
    int line_end; // expr can span multiple lines
    int char_start;
    int char_end;
    
    enum {
        EXPR_UNARY, 
        EXPR_BINARY,
        EXPR_FUNCTION_CALL,
        EXPR_LITERAL_INT
    } type;

    union {
        struct {
            enum {
                OP_UNARY_NEGATE,
                OP_UNARY_NOT,
                OP_UNARY_REFERENCE,
                OP_UNARY_DEREFERENCE,
                OP_UNARY_PAREN
            } operator;
        
            struct Expr *operand;
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

            struct Expr *lhs;
            struct Expr *rhs;
        } binary;

        struct {
            char *id;
            struct Expr *params;
            int param_count;
        } function_call;

        int literal_int;

    } data;
} Expr;

bool parse_expr(Lexer *lexer, Expr *expr);
void expr_free(Expr *expr);
void expr_print(Expr *expr);
#endif
