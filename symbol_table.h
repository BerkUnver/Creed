#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include "parser.h"
#include <stdbool.h>

typedef struct ExprResult {
    Type type;
    enum {
        EXPR_RESULT_CONSTANT,
        EXPR_RESULT_LVAL,
        EXPR_RESULT_RVAL
    } state;
} ExprResult;

void expr_result_free(ExprResult *result);

typedef struct SymbolTableNode {
    Declaration **declarations;
    int declaration_count;
    int declaration_count_alloc;
} SymbolTableNode;

#define SYMBOL_TABLE_NODE_COUNT 64
typedef struct SymbolTable {
    SymbolTableNode nodes[SYMBOL_TABLE_NODE_COUNT];
    struct SymbolTable *previous;
} SymbolTable;

void symbol_table_new(SymbolTable *out, SymbolTable *previous);
void symbol_table_free(SymbolTable *table);

bool symbol_table_insert(SymbolTable *table, Declaration *decl);
Declaration *symbol_table_get(SymbolTable *table, StringId id);
void symbol_table_resolve_type(SymbolTable *table, Type *type);
ExprResult symbol_table_check_expr(SymbolTable *table, Expr *expr);
void symbol_table_check_scope(SymbolTable *table, Scope *scope, Type *return_type);

void typecheck(SourceFile *file);
#endif
