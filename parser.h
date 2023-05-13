#ifndef CREED_PARSER_H
#define CREED_PARSER_H

#include <stdbool.h>
#include "lexer.h"
#include "token.h"

struct Declaration;

typedef struct Type {
    Location location;

    enum {
        TYPE_PRIMITIVE,
        TYPE_ID,
        TYPE_PTR,
        TYPE_PTR_NULLABLE,
        TYPE_ARRAY,
        TYPE_FUNCTION
    } type;

    union {
        TokenType primitive;
        
        struct Type *sub_type;
        
        struct {
            StringId type_declaration_id;
            struct Declaration *type_declaration;
        } id;

        struct {
            struct Type *params;
            int param_count;
            struct Type *result;
        } function;
    } data;
} Type;

Type type_parse(Lexer *lexer);
void type_print(Type *type);
void type_free(Type *type);

struct Scope;

typedef struct FunctionParameter {
    Location location;
    StringId id;
    Type type;
} FunctionParameter;

typedef struct Expr {
    Location location;
    
    enum {
        EXPR_PAREN,
        EXPR_UNARY, 
        EXPR_BINARY,
        EXPR_TYPECAST,
        EXPR_ACCESS_MEMBER,
        EXPR_ACCESS_ARRAY,
        EXPR_FUNCTION,
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
            FunctionParameter *params;
            int param_count;
            Type result;
            struct Scope *scope; // has to be a ptr because Scope contains expressions.
        } function;

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
void expr_print(Expr *expr, int indent);

typedef struct MemberStructUnion {
    Location location;
    StringId id;
    Type type;
} MemberStructUnion;

typedef struct MemberSum {
    Location location;
    StringId id;
    bool type_exists;
    Type type;
} MemberSum;

typedef struct Declaration {
    Location location;
    StringId id;
    enum {
       DECLARATION_VAR,
       DECLARATION_ENUM,
       DECLARATION_STRUCT,
       DECLARATION_UNION,
       DECLARATION_SUM
    } type;

    enum {
        DECLARATION_STATE_UNINITIALIZED,
        DECLARATION_STATE_INITIALIZING,
        DECLARATION_STATE_INITIALIZED
    } state;

    union {
        struct {
            enum {
                DECLARATION_VAR_CONSTANT,
                DECLARATION_VAR_MUTABLE,
            } type;

            union {
                struct {
                    bool type_exists;
                    Type type;
                    Expr value;
                } constant;
                
                struct {
                    Type type;
                    bool value_exists;
                    Expr value;
                } mutable; // This is syntax-highlighted in vim because 'mutable' is a c++ keyword and it can't differenciate between c++ headers and c headers.
            } data;
        } var;
        
        struct {
            MemberStructUnion *members;
            int member_count;
        } struct_union;
        
        struct {
            MemberSum *members;
            int member_count;
        } sum;

        struct {
            StringId *members;
            int member_count;
        } enumeration;
    } data;
} Declaration;

Declaration declaration_parse(Lexer *lexer);
void declaration_free(Declaration *decl);
void declaration_print(Declaration *decl, int indent);

typedef struct Statement {
    Location location;
    
    enum {
        STATEMENT_DECLARATION, 
        STATEMENT_INCREMENT,
        STATEMENT_DEINCREMENT,
        STATEMENT_ASSIGN,
        STATEMENT_EXPR,
        STATEMENT_LABEL,
        STATEMENT_LABEL_GOTO,
        STATEMENT_RETURN
    } type;

    union {
        Declaration declaration;
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
void statement_print(Statement *statement, int indent);

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
#endif
