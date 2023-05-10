typedef struct SymbolTableNode {
    Symbol *symbols;
    int symbol_count;
    int symbol_count_alloc;
} SymbolTableNode;

#define SYMBOL_TABLE_NODE_COUNT 64
typedef struct SymbolTable {
    TypeInfo *types;
    int type_count;
    SymbolTableNode nodes[SYMBOL_TABLE_NODE_COUNT];
} SymbolTable;
