#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include "parser.h"
#include <stdbool.h>

typedef struct ExprResult {
    Type type;
    bool is_lval;
    bool is_constant;
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

bool symbol_table_insert(SymbolTable *table, Declaration *decl);
Declaration *symbol_table_get(SymbolTable *table, StringId id);
void symbol_table_resolve_type(SymbolTable *table, Type *type);
ExprResult symbol_table_check_expr(SymbolTable *table, Expr *expr);
void symbol_table_check_scope(SymbolTable *table, Scope *scope);
void symbol_table_free(SymbolTable *table);

void typecheck(SourceFile *file);
#endif
