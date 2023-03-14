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
            enum { // eventually assign all as directly equal to a token type.            
                OP_BINARY_ADD = TOKEN_OP_PLUS,
                OP_BINARY_SUBTRACT = TOKEN_OP_MINUS,
                OP_BINARY_MULTIPLY = TOKEN_OP_MULTIPLY,
                OP_BINARY_DIVIDE = TOKEN_OP_DIVIDE,
                OP_BINARY_MODULO = TOKEN_OP_MODULO,
                
                OP_BINARY_GT,
                OP_BINARY_LT,
                OP_BINARY_GE,
                OP_BINARY_LE,

                OP_BINARY_LOGICAL_AND,
                OP_BINARY_LOGICAL_OR,
                
                OP_BINARY_EQ = TOKEN_OP_EQ,
                OP_BINARY_NE,
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
