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

#define SYMBOL_TABLE_FOR_EACH(statement, table) \
    for (int node_idx = 0; node_idx < SYMBOL_TABLE_NODE_COUNT; node_idx++) \
    for (int statement_idx = (statement = (file)->nodes[node_idx].statement_count, 0); statement_idx < (file)->nodes[node_idx].statement_count; statement_idx++, statement++)
