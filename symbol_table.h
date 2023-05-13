#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include "parser.h"

typedef struct SymbolTableNode {
    Declaration **declarations;
    int declaration_count;
    int declaration_count_alloc;
} SymbolTableNode;

#define SYMBOL_TABLE_NODE_COUNT 64
typedef struct SymbolTable {
    SymbolTableNode nodes[SYMBOL_TABLE_NODE_COUNT];
} SymbolTable;

SymbolTable symbol_table_new(StringId path);
void symbol_table_free(SymbolTable *table);
#endif
