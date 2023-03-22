#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"
#include "token.h"

#define STR_INDENTATION "   "
typedef struct Type {
    Location location;

    enum {
        TYPE_ID,
        TYPE_PTR,
        TYPE_PTR_NULLABLE,
        TYPE_ARRAY,
    } type;

    union {
        struct Type *sub_type;
        char *id;
    } data;
} Type;

Type type_parse(Lexer *lexer);
void type_print(Type *type);
void type_free(Type *type);

typedef struct Expr {
    Location location;
    
    enum {
        EXPR_UNARY, 
        EXPR_BINARY,
        EXPR_TYPECAST,
        EXPR_FUNCTION_CALL,
        EXPR_ID,
        EXPR_LITERAL,
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
            struct Expr *function;
            struct Expr *params;
            int param_count;
        } function_call;

        struct {
            struct Expr *operand;
            Type cast_to;
        } typecast;

        Literal literal;
        char *id;
    } data;
} Expr;

Expr expr_parse(Lexer *lexer);
void expr_free(Expr *expr);
void expr_print(Expr *expr);

typedef struct Statement {
    Location location;
    
    enum {
        STATEMENT_VAR_DECLARE,
    } type;

    union {
        struct {
            Type type;
            char *id;
            bool has_assign;
            Expr assign;
        } var_declare;
    } data;
} Statement;

Statement statement_parse(Lexer *lexer);
void statement_free(Statement *statement);
void statement_print(Statement *statement);

typedef struct Scope {
    Location location;
    int statement_count;
    Statement *statements;
} Scope;

Scope scope_parse(Lexer *lexer);
void scope_free(Scope *scope);
void scope_print(Scope *scope, int indentation);

#endif
