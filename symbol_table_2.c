#include <stdlib.h>
#include "symbol_table_2.h"

SymbolTable symbol_table_new(StringId path) {
    SymbolTable table;
    memset(&table, 0, sizeof(SymbolTable));
    
    Lexer lexer = lexer_new(string_cache_get(path));

    // No imports for now.
    
    while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
        Declaration *decl = declaration_parse(&lexer);
        if (lexer_token_get(lexer).type != TOKEN_SEMICOLON) {
            error_exit(decl.location, "Expected a semicolon after a declaration.");
        }

        int idx = decl.id.idx % SYMBOL_TABLE_NODE_COUNT;
        for (int i = 0; i < table.nodes[idx].declaration_count; i++) {
            if (table.nodes[idx].declarations[i].id.idx == decl.id.idx) {
                error_exit(decl.location, "A declaration with this name already exists.");
            }
        }

        table.nodes[idx].declaration_count++;
        if (table.nodes[idx].declaration_count > table.nodes[idx].declaration_count_alloc) {
            if (table.nodes[idx].declaration_count_alloc == 0) {
                table.nodes[idx].declaration_count_alloc = 2;
            } else {
                table.nodes[idx].declaration_count_alloc *= 2;
            }
            
            table.nodes[idx].declarations = realloc(table.nodes[idx].symbols, sizeof(Symbol) * table.nodes[idx].symbol_count_alloc);
        }
        table.nodes[idx].declarations[table.nodes[idx].symbol_count - 1] = decl;
    }
    lexer_free(&lexer);
    

    int type_count = 0;
    int type_count_alloc = 2;
    TypeInfo *types = malloc(sizeof(TypeInfo) * type_count_alloc);
    
    Declaration *decl;
    SYMBOL_TABLE_FOR_EACH(decl, table) {
        if (decl->type == DECLARATION_VAR) continue;
        type_count++;
        if (type_count > type_count_alloc) {
            type_count_alloc *= 2;
            types = realloc(types, sizeof(TypeInfo) * type_count_alloc);
        }
        types[type_count - 1] = (TypeInfo) {
            .state = TYPE_INFO_STATE_UNINITIALIZED;
        }
        decl->type_info_idx = type_count - 1;
    }

    /*
    SYMBOL_TABLE_FOR_EACH(decl, table) {
        if (decl->type == DECLARATION_VAR) continue;
        switch (decl->type) {
            case DECLARATION_VAR:
        }
    }
    */
}
