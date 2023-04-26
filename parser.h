#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"
#include "token.h"

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
        EXPR_ACCESS_MEMBER,
        EXPR_ACCESS_ARRAY,
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
                EXPR_UNARY_BITWISE_NOT = '~',
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
        } access_member;
        
        struct {
            struct Expr *operand;
            struct Expr *index;
        } access_array;

        Literal literal;
        bool literal_bool;
        StringId id;
    } data;
} Expr;

Expr expr_parse(Lexer *lexer);
void expr_free(Expr *expr);
void expr_print(Expr *expr);

typedef struct Statement {
    Location location;
    
    enum {
        STATEMENT_VAR_DECLARE, 
        STATEMENT_INCREMENT,
        STATEMENT_DEINCREMENT,
        STATEMENT_ASSIGN,
        STATEMENT_EXPR,
        STATEMENT_LABEL,
        STATEMENT_LABEL_GOTO,
        STATEMENT_RETURN
    } type;

    union {
        struct {
            Type type;
            StringId id;
            bool has_assign;
            Expr assign;
        } var_declare;

        Expr increment;
        Expr deincrement;
        
        struct {
            Expr assignee;
            Expr value;
            TokenType type;
        } assign;

        Expr expr;

        StringId label;
        StringId label_goto;
        Expr return_expr;
    } data;
} Statement;

struct Scope;

typedef struct MatchCase {
    Location location;
    StringId match_id;
    bool declares; // if the case creates a new variable
    Statement declared_var; // id for created variable
    int scope_count;
    struct Scope *scopes;
} MatchCase;

Statement statement_parse(Lexer *lexer);
void statement_free(Statement *statement);
void statement_print(Statement *statement);

typedef struct Scope {
    Location location;
    enum {
        SCOPE_BLOCK,
        SCOPE_STATEMENT,
        SCOPE_CONDITIONAL,
        SCOPE_LOOP_FOR,
        SCOPE_LOOP_FOR_EACH,
        SCOPE_LOOP_WHILE,
        SCOPE_MATCH,
    } type;

    union {
        Statement statement;

        struct {
            Statement init;
            Expr expr;
            Statement step;
            struct Scope *scope;
        } loop_for;
        
        struct {
            Expr condition;
            struct Scope *scope_if;
            struct Scope *scope_else; // if null then no else statement exists.
        } conditional;

        struct {
            StringId element;
            Expr array;
            struct Scope *scope;
        } loop_for_each;

        struct {
            Expr expr;
            struct Scope *scope;
        } loop_while;

        struct {
            struct Scope *scopes;
            int scope_count;
        } block;

        struct {
            Expr expr;
            int case_count;
            MatchCase *cases;
        } match;
    } data;
} Scope;

Scope scope_parse(Lexer *lexer);
void scope_free(Scope *scope);
void scope_print(Scope *scope, int indentation);

typedef struct FunctionParameter {
    StringId id;
    Type type;
} FunctionParameter;


typedef struct MemberStructUnion {
    StringId id;
    Type type;
} MemberStructUnion;

typedef struct MemberSum {
    StringId id;
    bool type_exists;
    Type type;
} MemberSum;

typedef struct Declaration {
    Location location;
    StringId id;
    
    enum {
        DECLARATION_STRUCT,
        DECLARATION_UNION,
        DECLARATION_SUM,
        DECLARATION_ENUM,
        DECLARATION_FUNCTION,
        // DECLARATION_CONSTANT
        
        /* We need to figure out how to handle globals much better.
         * Right now, we just have constants as literals, i.e.
         * 
         * CONSTANT_1 128
         * CONSTANT_2 256
         * 
         * We cannot hande the addition of constants, i.e.
         * CONSTANT_3 CONSTANT_1 + CONSTANT_2
         * 
         * We could handle that, but then it introduces a syntactical ambiguity between function arguments and parenthesized expressions.
         
         * CONSTANT_3 (CONSTANT_1 + CONSTANT_2)
         * main () void { }
         
         * I believe differenciating between these would make it an LR grammar.
         * A way to resolve this would be to add a "fn" keyword before every function. I'm not opposed to this, and also makes lamdas and function type declarations easier.
         *
         * main fn () void { }
         *
         * However, there is another issue. How do we resolve mutable globals?
         * We could just parse them like normal declarations.
         *
         * global_1: int = 3;
         *
         * However, this would require semicolons after them and requiring semicolons after globals but not constants is weird.
         *
         * CONSTANT_1 128;
         *
         * Does this imply having semicolons after type and function declarations?
         * Right now, the following is legal.
         *
         * main () int return EXIT_FAILURE;
         *
         * We would have to require 2 semicolons after a single-line function declaration, one for the function body and for the function declaration itself.
         *
         * main () int return EXIT_FAILURE;;
         * 
         * This is really, really dumb, and avoiding semicolons after function declarations would be good.
         * However, this would also mean function declarations would have to imply the end of a statement when direcly assigned to a constant name.
         * This might imply something weird about lambdas.
         *
         * I feel like not having a constant assignment operator is just asking for syntactic ambiguities at this point as well.
         *
         * main :: fn () void { }
         */
    } type;

    union {
        struct {
            MemberStructUnion *members;
            int member_count;
        } d_struct_union;
        
        struct {
            MemberSum *members;
            int member_count;
        } d_sum;

        struct {
            StringId *members;
            int member_count;
        } d_enum;
        
        struct {
            FunctionParameter *parameters;
            int parameter_count;
            Type return_type;
            Scope scope;
        } d_function;
    } data;

} Declaration;

Declaration declaration_parse(Lexer *lexer);
void declaration_free(Declaration *declaration);
void declaration_print(Declaration *declaration);

#endif
