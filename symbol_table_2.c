#include <stdlib.h>
#include "symbol_table_2.h"

SymbolTable symbol_table_new(StringId path) {
    SymbolTable table;
    memset(&table, 0, sizeof(SymbolTable));
    
    Lexer lexer = lexer_new(string_cache_get(path));

    // No imports for now.
    
    while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
        Statement statement = statement_parse(&lexer);
        int idx 
        int idx = statement.id.idx % SYMBOL_TABLE_NODE_COUNT;
        for (int i = 0; i < table.nodes[idx].symbol_count; i++) {
            if (table.nodes[idx].symbols[i].id.idx == statement.id.idx) {
                error_exit(statement.location, "A declaration with this name already exists.");
            }
        }

        table.nodes[idx].symbol_count++;
        if (table.nodes[idx].symbol_count > table.nodes[idx].symbol_count_alloc) {
            if (table.nodes[idx].symbol_count_alloc == 0) {
                table.nodes[idx].symbol_count_alloc = 2;
            } else {
                table.nodes[idx].symbol_count_alloc *= 2;
            }
            
            table.nodes[idx].symbols = realloc(table.nodes[idx].symbols, sizeof(Symbol) * table.nodes[idx].symbol_count_alloc);
        }
        table.nodes[idx].symbols[table.nodes[idx].symbol_count - 1] = decl;
    }
    lexer_free(&lexer);
    

    Statement *statement
    SYMBOL_TABLE_FOR_EACH(
}
