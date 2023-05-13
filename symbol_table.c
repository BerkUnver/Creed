#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "symbol_table.h"

bool symbol_table_insert(SymbolTable *table, Declaration decl) {
    int idx = decl.id.idx % SYMBOL_TABLE_NODE_COUNT;
    for (int i = 0; i < table->nodes[idx].declaration_count; i++) {
        if (table->nodes[idx].declarations[i]->id.idx == decl.id.idx) return false;
    }

    table->nodes[idx].declaration_count++;
    if (table->nodes[idx].declaration_count > table->nodes[idx].declaration_count_alloc) {
        if (table->nodes[idx].declaration_count_alloc == 0) {
            table->nodes[idx].declaration_count_alloc = 2;
        } else {
            table->nodes[idx].declaration_count_alloc *= 2;
        }
        
        table->nodes[idx].declarations = realloc(table->nodes[idx].declarations, sizeof(Declaration *) * table->nodes[idx].declaration_count_alloc);
    }

    Declaration *decl_ptr = malloc(sizeof(Declaration));
    *decl_ptr = decl;
    table->nodes[idx].declarations[table->nodes[idx].declaration_count - 1] = decl_ptr;
    return true;
}

Declaration *symbol_table_get(SymbolTable table, StringId id) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    for (int i = 0; i < table.nodes[idx].declaration_count; i++) {
        if (table.nodes[idx].declarations[i]->id.idx == id.idx) {
            return table.nodes[idx].declarations[i];
        }
    }
    return NULL;
}

static void symbol_table_declaration_init(SymbolTable table, Declaration *decl) {
    switch (decl->state) {
        case DECLARATION_STATE_UNINITIALIZED:
            switch (decl->type) {
                case DECLARATION_VAR: break;

                case DECLARATION_ENUM: {
                    error_exit(decl->location, "Enums are currently not supported.");
                } break;

                case DECLARATION_STRUCT:
                case DECLARATION_UNION: {
                    decl->state = DECLARATION_STATE_INITIALIZING;

                    for (int i = 0; i < decl->data.struct_union.member_count; i++) {
                        StringId member_id = decl->data.struct_union.members[i].id;
                        for (int j = 0; j < decl->data.struct_union.member_count; j++) {
                            if (i == j) continue;
                            if (decl->data.struct_union.members[j].id.idx == member_id.idx) {
                                error_exit(decl->location, "This struct contains duplicate members.");
                            }
                        }

                        switch (decl->data.struct_union.members[i].type.type) {
                            case TYPE_PRIMITIVE: 
                                break;
                            case TYPE_ID: {
                                Declaration *decl_member_type = symbol_table_get(table, decl->data.struct_union.members[i].type.data.id.type_declaration_id);
                                if (!decl_member_type) {
                                    error_exit(decl->data.struct_union.members[i].type.location, "This type does not exist in the current scope.");
                                }
                                if (decl_member_type->type == DECLARATION_VAR) {
                                    error_exit(decl->data.struct_union.members[i].type.location, "This is the name of a variable, not a type.");
                                }
                                symbol_table_declaration_init(table, decl_member_type);
                                decl->data.struct_union.members[i].type.data.id.type_declaration = decl_member_type; 
                            } break;

                            case TYPE_PTR:
                            case TYPE_PTR_NULLABLE:
                            case TYPE_ARRAY: /*{
                                Type *sub_type = decl->data.struct_union.members[i].type.sub_type;
                                while (sub_type->type == TYPE_PTR || sub_type->type == TYPE_PTR_NULLABLE 
                            }*/
                            case TYPE_FUNCTION:
                                assert(false);
                        }
                    }
                    decl->state = DECLARATION_STATE_INITIALIZED;
                } break;

                case DECLARATION_SUM: {
                    error_exit(decl->location, "Sum types are currently not supported.");
                } break;
            } break;

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
    
    while (lexer_token_peek(&lexer).type != TOKEN_NULL) {
        Declaration decl = declaration_parse(&lexer);
        if (lexer_token_get(&lexer).type != TOKEN_SEMICOLON) {
            error_exit(decl.location, "Expected a semicolon after a declaration.");
        }
        
        if (!symbol_table_insert(&table, decl)) {
            error_exit(decl.location, "This declaration has a duplicate name.");
        }
    }
    
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++)
    for (int j = 0; j < table.nodes[i].declaration_count; j++) {
        Declaration *decl = table.nodes[i].declarations[j];
        symbol_table_declaration_init(table, decl);
    }

    return table;
}

void symbol_table_free(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++) {
        for (int j = 0; j < table->nodes[i].declaration_count; j++) {
            declaration_free(table->nodes[i].declarations[j]);
            free(table->nodes[i].declarations[j]);
        }
        free(table->nodes[i].declarations);
    }
}
