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


typedef struct TypeMember {
    StringId id;
    Type type;
} TypeMember;

typedef struct SumMember {
    StringId id;
    bool type_exists;
    Type type;
} SumMember;

typedef struct Declaration {
    Location location;
    StringId id;
    
    enum {
        // Eventually we will have to split sum away from struct and union in order to declare explicit tag values.
        DECLARATION_STRUCT,
        DECLARATION_UNION,
        DECLARATION_SUM,
        
        DECLARATION_ENUM,

        DECLARATION_FUNCTION,
        // DECLARATION_IMPORT,
        // DECLARATION_CONSTANT
    } type;

    union {
        struct { // shared by structs, sums, and unions.
            TypeMember *members;
            int member_count;
        } d_complex_type;
        
        struct {
            SumMember *members;
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

typedef struct SourceFile {
    StringId *imports;
    int import_count;

    Declaration *declarations;
    int declaration_count;
} SourceFile;

SourceFile source_file_parse(const char *file_name);
void source_file_print(SourceFile *file);
void source_file_free(SourceFile *file);

#endif
