#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "prelude.h"
#include "symbol_table.h"
#include "string_cache.h"
#include "token.h"

void symbol_table_new(SymbolTable *table) {
    memset(table->nodes, 0, sizeof(SymbolNode) * SYMBOL_TABLE_COUNT);
    table->previous = NULL;
}

bool symbol_table_has(SymbolTable *table, StringId id) {
    int idx = id.idx % SYMBOL_TABLE_COUNT;
    for (int i = 0; i < table->nodes[idx].count; i++) {
        if (table->nodes[idx].symbols[i].id.idx == id.idx) return true;
    }
    return false;
}

bool symbol_table_get(SymbolTable *table, StringId id, Symbol *symbol) {
    int i = id.idx % SYMBOL_TABLE_COUNT;
    for (int j = 0; j < table->nodes[j].count; j++) {
        if (table->nodes[i].symbols[j].id.idx != id.idx) continue;
        *symbol = table->nodes[i].symbols[j];
        return true;
    }
    return false;
}

bool symbol_table_add_var(SymbolTable *table, StringId id, Type *type) {
    assert(type->type == TYPE_PRIMITIVE); // temporary, just to get simple types working.
    if (symbol_table_has(table, id)) return false;
    
    Symbol symbol = {
        .id = id,
        .type = SYMBOL_VAR, 
        .type = type->data.primitive
    };
    
    int i = id.idx % SYMBOL_TABLE_COUNT;
    if (table->nodes[i].count == 0) {
        int count_alloc = 2;
        Symbol *symbols = malloc(sizeof(Symbol) * count_alloc);
        symbols[0] = symbol;
        table->nodes[i] = (SymbolNode) {
            .count = 1,
            .count_alloc = count_alloc,
            .symbols = symbols
        };
    } else {
        SymbolNode node = table->nodes[i];
        node.count++;
        if (node.count > node.count_alloc) {
            node.count_alloc *= 2;
            node.symbols = realloc(node.symbols, sizeof(Symbol) * node.count_alloc);
        }
        node.symbols[node.count - 1] = symbol;
        table->nodes[i] = node;
    }
    return true;
}

void symbol_table_print(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_COUNT; i++) {
        for (int j = 0; j < table->nodes[i].count; j++) {
            printf("%s: %s\n", string_cache_get(table->nodes[i].symbols[j].id), string_keywords[table->nodes[i].symbols[j].data.var_type - TOKEN_KEYWORD_MIN]);
        }
    }
}

void symbol_table_check_scope(SymbolTable *table, Scope *scope) {
    switch (scope->type) {
        case SCOPE_BLOCK: {
            SymbolTable table_new;
            memset(&table_new.nodes, 0, sizeof(SymbolNode) * SYMBOL_TABLE_COUNT);
            table_new.previous = table;
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                symbol_table_check_scope(&table_new, scope->data.block.scopes + i);
            }
        } break;
       
        case SCOPE_STATEMENT:
            switch (scope->data.statement.type) {
                case STATEMENT_VAR_DECLARE:
                    if (!symbol_table_add_var(table, scope->data.statement.data.var_declare.id, &scope->data.statement.data.var_declare.type)) {
                        error_exit(scope->location, "This local variable shadows another local variable in the same scope.");
                    }
                    break;
                default: assert(false);
            }
            break;
        case SCOPE_CONDITIONAL:
            symbol_table_check_scope(table, scope->data.conditional.scope_if);
            if (scope->data.conditional.scope_else) symbol_table_check_scope(table, scope->data.conditional.scope_else);
            break;

        default: assert(false);
    }
}

void symbol_table_check_declaration(Declaration *decl) {
    switch (decl->type) {
        case DECLARATION_FUNCTION: {
            SymbolTable table;
            symbol_table_new(&table);
            for (int i = 0; i < decl->data.d_function.parameter_count; i++) {
                FunctionParameter param = decl->data.d_function.parameters[i];
                if (!symbol_table_add_var(&table, param.id, &param.type)) {
                    error_exit(param.type.location, "This function parameter's name shadows another.");
                }
            }
            symbol_table_check_scope(&table, &decl->data.d_function.scope);
        } break;

        default: assert(false);
    }
}
