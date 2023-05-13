#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "symbol_table.h"

void expr_type_free(ExprType *type) {
    switch (type->type) {
        case EXPR_TYPE_PRIMITIVE:
        case EXPR_TYPE_DECLARATION:
            break;
        case EXPR_TYPE_PTR:
        case EXPR_TYPE_PTR_NULLABLE:
        case EXPR_TYPE_ARRAY:
            expr_type_free(type->data.sub_type);
            free(type->data.sub_type);
            break;
    }
}

bool symbol_table_insert(SymbolTable *table, Declaration *decl) {
    int idx = decl->id.idx % SYMBOL_TABLE_NODE_COUNT;
    for (int i = 0; i < table->nodes[idx].declaration_count; i++) {
        if (table->nodes[idx].declarations[i]->id.idx == decl->id.idx) return false;
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

    table->nodes[idx].declarations[table->nodes[idx].declaration_count - 1] = decl;
    return true;
}

Declaration *symbol_table_get(SymbolTable *table, StringId id) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    for (int i = 0; i < table->nodes[idx].declaration_count; i++) {
        if (table->nodes[idx].declarations[i]->id.idx == id.idx) {
            return table->nodes[idx].declarations[i];
        }
    }

    if (table->previous) return symbol_table_get(table->previous, id);
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
                                Declaration *decl_member_type = symbol_table_get(&table, decl->data.struct_union.members[i].type.data.id.type_declaration_id);
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
                            case TYPE_ARRAY:
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

/*
ExprType symbol_table_check_expr(SymbolTable *table, Expr *expr) {
    switch (expr->type) {
        case EXPR_PAREN:
            return symbol_table_check_expr(table, expr->data.parenthesized);
        case EXPR_UNARY: {
            ExprType expr_type = symbol_table_check_expr(table, expr->data.unary.operand);
            if (expr_type.type != EXPR_TYPE_PRIMITIVE) {
                error_exit(expr->location, "The operand of a unary expression must have a primitive type.");
            }
            TokenType type = expr_type.data.primitive;
            switch (expr->data.unary.type) {
                case EXPR_UNARY_LOGICAL_NOT:
                    if (type != TOKEN_KEYWORD_TYPE_BOOL) error_exit(expr->location, "The operand of a negation expression must have a boolean type.");
                    return expr_type;
                default: assert(false);
            }
        } break;

        case EXPR_BINARY: {
            ExprType expr_type_lhs = symbol_table_check_expr(table, expr->data.binary.lhs);
            ExprType expr_type_rhs = symbol_table_check_expr(table, expr->data.binary.rhs);
            if (expr_type_lhs.type != EXPR_TYPE_PRIMITIVE || expr_type_rhs.type != EXPR_TYPE_PRIMITIVE) {
                error_exit(expr->location, "The operands of a binary expression must both be of a primitive type.");
            }
            // unfinished
        } break;

        default: assert(false);
    }
}

void symbol_table_check_scope(SymbolTable *table, Scope *scope) { 
    switch (scope->type) {
        case SCOPE_BLOCK: {
            SymbolTable table_scope;
            memset(&table_scope, 0, sizeof(SymbolTable));
            table_scope.previous = table;
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                symbol_table_check_scope(&table_scope, scope->data.block.scopes + i);
            }
        } break;

        case SCOPE_STATEMENT: {
            switch (scope->data.statement.type) {
                case STATEMENT_DECLARATION: assert(false);
                case STATEMENT_INCREMENT: {
                    Declaration *decl = symbol_table_get(table, scope->data.statement.data.increment);
                } break; 
                case STATEMENT_DEINCREMENT:
            }
        } break;
    }
}
*/

void symbol_table_free(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++) {
        free(table->nodes[i].declarations);
    }
}

void typecheck(SourceFile *file) {
    SymbolTable table;
    memset(&table, 0, sizeof(SymbolTable));
   
    for (int i = 0; i < file->declaration_count; i++) {
        if (symbol_table_insert(&table, file->declarations + i)) continue;
        error_exit(file->declarations[i].location, "This declaration has a duplicate name.");
    }
    
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++)
    for (int j = 0; j < table.nodes[i].declaration_count; j++) {
        Declaration *decl = table.nodes[i].declarations[j];
        symbol_table_declaration_init(table, decl);
    }

    symbol_table_free(&table);
}
