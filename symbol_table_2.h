typedef struct SymbolTableNode {
    Declaration **declarations;
    int declaration_count;
    int declaration_count_alloc;
} SymbolTableNode;

#define SYMBOL_TABLE_NODE_COUNT 64
typedef struct SymbolTable {
    SymbolTableNode nodes[SYMBOL_TABLE_NODE_COUNT];
} SymbolTable;

#define SYMBOL_TABLE_FOR_EACH(decl, table) \
    for (int node_idx = 0; node_idx < SYMBOL_TABLE_NODE_COUNT; node_idx++) \
    for (int decl_idx = (decl = (file)->nodes[node_idx].declarations[0], 0); decl_idx < (file)->nodes[node_idx].declaration_count; decl_idx++, decl = (file)->nodes[node_idx].declarations[decl_idx]) \
