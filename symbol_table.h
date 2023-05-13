#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include "parser.h"

typedef struct ExprType {
    bool is_rval;
    
    enum {
        EXPR_TYPE_PRIMITIVE,
        EXPR_TYPE_DECLARATION,
        EXPR_TYPE_PTR,
        EXPR_TYPE_PTR_NULLABLE,
        EXPR_TYPE_ARRAY,
    } type;

    union {
        TokenType primitive;
        Declaration *declaration;
        struct ExprType *sub_type;
    } data;
} ExprType;

void expr_type_free(ExprType *type);

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

void symbol_table_free(SymbolTable *table);
void typecheck(SourceFile *file);
#endif
