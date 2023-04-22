#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include "stdbool.h"
#include "string_cache.h"
#include "token.h"

typedef struct Symbol {
    StringId id;

    enum {
        SYMBOL_VAR
    } type;

    union {
        TokenType var_type;
    } data;
} Symbol;

typedef struct SymbolNode {
    int count;
    int count_alloc;
    Symbol *symbols;
} SymbolNode;

#define SYMBOL_TABLE_COUNT 128
typedef struct SymbolTable {
    SymbolNode nodes[SYMBOL_TABLE_COUNT];
    struct SymbolTable *previous;
} SymbolTable;

void symbol_table_new(SymbolTable *table);
bool symbol_table_has(SymbolTable *table, StringId id);
bool symbol_table_get(SymbolTable *table, StringId id, Symbol *symbol);
bool symbol_table_add_var(SymbolTable *table, StringId id, Type *type);
void symbol_table_print(SymbolTable *table);
void symbol_table_check_scope(SymbolTable *table, Scope *scope);
void symbol_table_check_declaration(Declaration *decl);
#endif
