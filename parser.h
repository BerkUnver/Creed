#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"
#include "token.h"

#define STR_INDENTATION "   "
typedef struct Type {
    Location location;

    enum {
        TYPE_PRIMITIVE,
        TYPE_ID,
        TYPE_PTR,
        TYPE_PTR_NULLABLE,
        TYPE_ARRAY,
    } type;

    union {
        TokenType primitive;
        struct Type *sub_type;
        StringId id;
    } data;
} Type;

Type type_parse(Lexer *lexer);
void type_print(Type *type);
void type_free(Type *type);

typedef struct Expr {
    Location location;
    
    enum {
        EXPR_PAREN,
        EXPR_UNARY, 
        EXPR_BINARY,
        EXPR_TYPECAST,
        EXPR_MEMBER_ACCESS,
        EXPR_FUNCTION_CALL,
        EXPR_ID,
        EXPR_LITERAL,
        EXPR_LITERAL_BOOL
    } type;

    union {
        struct Expr *parenthesized;

        struct {
            enum {
                EXPR_UNARY_LOGICAL_NOT = '!',
                EXPR_UNARY_BITWISE_NOT = '^',
                EXPR_UNARY_NEGATE = '-',
                EXPR_UNARY_REF = '&',
                EXPR_UNARY_DEREF = '*',
            } type; 
            
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

        struct {
            struct Expr *operand;
            StringId member;
        } member_access;

        Literal literal;
        bool literal_bool;
        StringId id;
    } data;
} Expr;

Expr expr_parse(Lexer *lexer);
void expr_free(Expr *expr);
void expr_print(Expr *expr);

struct Statement;

typedef struct Scope {
    Location location;
    int statement_count;
    struct Statement *statements;
} Scope;

Scope scope_parse(Lexer *lexer);
void scope_free(Scope *scope);
void scope_print(Scope *scope, int indentation);

typedef struct Statement {
    Location location;
    
    enum {
        STATEMENT_VAR_DECLARE, 
        STATEMENT_LOOP_FOR,// TODO: add conditionals, loops, matches ect. (tom)
        STATEMENT_LOOP_WHILE,
    } type;

    union {
        struct {
            Type type;
            StringId id;
            bool has_assign;
            Expr assign;
        } var_declare;

        struct {
            struct Statement *init;
            Expr expr;
            struct Statement *step;
            Scope scope;
        } loop_for;

        struct {
           Expr condition;
           Scope body;
        } loop_while;

    } data;
} Statement;

Statement statement_parse(Lexer *lexer);
void statement_free(Statement *statement);
void statement_print(Statement *statement, int indentation);

typedef struct Constant {
    Location location;

    enum {
        CONSTANT_FUNCTION
    } type;

    union {
        struct {
            StringId name;
            int parameter_count;
            struct {StringId name; Type type; } *parameters;
            Scope scope;
        } function;
    } data;
} Constant;

Constant constant_parse(Lexer *lexer);
void constant_free(Constant *constant);
#endif
