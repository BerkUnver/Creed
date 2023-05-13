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

Declaration *symbol_table_get(SymbolTable table, StringId id) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    for (int i = 0; i < table.nodes[idx].declaration_count; i++) {
        if (table.nodes[idx].declarations[i]->id.idx == id.idx) {
            return table.nodes[idx].declarations[i];
        }
    }
}

static void symbol_table_declaration_init(SymbolTable table; Declaration *decl) {
    switch (decl->state) {
        case DECLARATION_STATE_UNINTIALIZED:
            switch (decl->type) {
                case DECLARATION_VAR: break;

                case DECLARATION_ENUM: {
                    error_exit(decl->location, "Enums are currently not supported.");
                } break;

                case DECLARATION_STRUCT:
                case DECLARATION_UNION: {
                    decl->state = DECLARATION_STATE_INITIALZING;

                    for (int i = 0; i < member_count; i++) {
                        StringId id = decl->data.struct_union.members[i].id;
                        for (int j = 0; j < member_count; j++) {
                            if (i == j) continue;
                            if (decl->data.struct_union.members[j].idx == id.idx) {
                                error_exit(decl->location, "This struct contains duplicate members.");
                            }
                        }

                        switch (decl->data.struct_union.members[i].type.type) {
                            case TYPE_PRIMITIVE: 
                                break;
                            case TYPE_ID: {
                                Declaration *decl_member_type = symbol_table_get(table, id);
                                if (!decl_member_type) {
                                    error_exit(decl->data.struct_union.members[i].type.location, "This type does not exist in the current scope.");
                                }
                                if (decl_member_type->type == DECLARATION_VAR) {
                                    error_exit(decl->data.struct_union.members[i].type.location, "This is the name of a variable, not a type.");
                                }
                                symbol_table_declaration_init(table, decl_member_type);
                            } break;

                            case TYPE_PTR:
                            case TYPE_PTR_NULLABLE:
                            case TYPE_ARRAY:
                            case TYPE_FUNCTION:
                                assert(false);
                        }
                    }
                    decl->state = DECLARATIO_STATE_INITIALIZED;
                } break;

                case DECLARATION_SUM: {
                    error_exit(decl->location, "Sum types are currently not supported.");
                } break;
            }
        case DECLARATION_STATE_INITIALIZING: 
            error_exit(decl->location, "This complex type contains a circular reference.");
            break;
        case DECLARATION_STATE_INITIALIZED:
            break;
    }
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
    
    Declaration *decl;
    SYMBOL_TABLE_FOR_EACH(decl, table) {
        symbol_table_declaration_init(table, decl);
    }
}
