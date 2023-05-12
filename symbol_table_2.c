#include <stdlib.h>
#include "symbol_table_2.h"

bool symbol_table_insert(SymbolTable table, Declaration decl) {
    int idx = decl.id.idx % SYMBOL_TABLE_NODE_COUNT;
    for (int i = 0; i < table.nodes[idx].declaration_count; i++) {
        if (table.nodes[idx].declarations[i].id.idx == decl.id.idx) return false;
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
    return true;
}

SymbolTable symbol_table_new(StringId path) {
    SymbolTable table;
    memset(&table, 0, sizeof(SymbolTable));
    
    Lexer lexer = lexer_new(string_cache_get(path));

    // No imports for now.
    
    while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
        Declaration decl = declaration_parse(&lexer);
        if (lexer_token_get(lexer).type != TOKEN_SEMICOLON) {
            error_exit(decl.location, "Expected a semicolon after a declaration.");
        }
        
        if (!symbol_table_insert(decl)) {
            error_exit(decl->location, "This declaration has a duplicate name.");
        }
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

    SYMBOL_TABLE_FOR_EACH(decl, table) {
        switch (decl->type) {
            case DECLARATION_VAR:
                break;

            case DECLARATION_ENUM: {
                int member_count = decl->data.enumeration.member_count;
                StringId *members = malloc(sizeof(StringId) * member_count);
                for (int i = 0; i < member_count; i++) {
                    StringId member = decl->data.enumeration.members[i];
                    for (int j = 0; j < member_count; j++) {
                        if (i == j) continue;
                        if (decl->data.enumeration.members[j].idx == member.idx) {
                            error_exit(decl->location, "This enum contains duplicate members.");
                        }
                    }
                    members[i] = member;
                }
                
                types[decl->type_info_idx].data->enumeration.member_count = member_count;
                types[decl->type_info_idx].data->enumeration.members = members;
                
            } break;
        }
    }
}
