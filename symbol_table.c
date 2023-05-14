#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"

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

void symbol_table_resolve_type(SymbolTable *table, Type *type) {
    switch (type->type) {
        case TYPE_PRIMITIVE:
            break;

        case TYPE_ID: {
            Declaration *decl = symbol_table_get(table, type->data.id.type_declaration_id);
            if (!decl) error_exit(type->location, "This type does not exist in the current scope.");
            if (decl->type == DECLARATION_VAR) error_exit(type->location, "This is the name of a variable, not a type.");
            type->data.id.type_declaration = decl;
        } break;

        case TYPE_PTR:
        case TYPE_PTR_NULLABLE:
        case TYPE_ARRAY:
            symbol_table_resolve_type(table, type->data.sub_type);
            break;

        case TYPE_FUNCTION: {
            int param_count = type->data.function.param_count;
            for (int i = 0; i < param_count; i++) {
                symbol_table_resolve_type(table, type->data.function.params + i);
            }
            symbol_table_resolve_type(table, type->data.function.result);
        } break;
    }
}

static void symbol_table_declaration_init(SymbolTable *table, Declaration *decl) {
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

                        symbol_table_resolve_type(table, &decl->data.struct_union.members[i].type);
                        
                        if (decl->data.struct_union.members[i].type.type == TYPE_ID) {
                            Declaration *decl_member_type = symbol_table_get(table, decl->data.struct_union.members[i].type.data.id.type_declaration_id);
                            assert(decl_member_type && decl_member_type->type != DECLARATION_VAR); // Guaranteed to be true if resolving the type worked correctly.
                            symbol_table_declaration_init(table, decl_member_type);
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

Type *symbol_table_check_expr(SymbolTable *table, Expr *expr, bool *is_rval, bool is_comptime) {
    switch (expr->type) {
        case EXPR_PAREN:
            return symbol_table_check_expr(table, expr->data.parenthesized, is_rval, is_comptime);
        case EXPR_UNARY: {
            bool rval_discard;
            Type *type = symbol_table_check_expr(table, expr->data.unary.operand, &rval_discard, is_comptime);
            *is_rval = false;

            if (type->type != TYPE_PRIMITIVE) {
                error_exit(expr->location, "The operand of a unary expression must have a primitive type.");
            }
            switch (expr->data.unary.type) {
                case EXPR_UNARY_LOGICAL_NOT:
                    if (type->data.primitive != TOKEN_KEYWORD_TYPE_BOOL) error_exit(expr->location, "The operand of a negation expression must have a boolean type.");
                    return type;
                default: assert(false);
            }
        } break;

        case EXPR_BINARY: { // unfinished
            bool rval_discard;
            Type *type_lhs = symbol_table_check_expr(table, expr->data.binary.lhs, &rval_discard, is_comptime);
            Type *type_rhs = symbol_table_check_expr(table, expr->data.binary.rhs, &rval_discard, is_comptime);
            
            if (type_lhs->type != TYPE_PRIMITIVE || type_rhs->type != TYPE_PRIMITIVE) {
                error_exit(expr->location, "The operands of a binary expression must both be of a primitive type.");
            }

            *is_rval = false;
            switch (expr->data.binary.operator) {
                case TOKEN_OP_LOGICAL_AND:
                case TOKEN_OP_LOGICAL_OR:
                    if (type_lhs->data.primitive != TOKEN_KEYWORD_TYPE_BOOL || type_rhs->data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                        error_exit(expr->location, "The operands of a logical operator are both expected to have a boolean type.");
                    }
                    return type_lhs;
                default: assert(false);
            }
        } break;
        
        case EXPR_TYPECAST: {
            bool rval_discard;
            Type *type = symbol_table_check_expr(table, expr->data.typecast.operand, &rval_discard, is_comptime);
            *is_rval = false;
            switch (type->type) {
                case TYPE_PRIMITIVE:
                    if (type->data.primitive == TOKEN_KEYWORD_TYPE_VOID) error_exit(expr->location, "You cannot cast a void expression.");
                    if (expr->data.typecast.cast_to.type != TYPE_PRIMITIVE) error_exit(expr->location, "You can only cast a primitive type to another primitve type.");
                    break;

                case TYPE_PTR:
                case TYPE_PTR_NULLABLE:
                    if (expr->data.typecast.cast_to.type != TYPE_PTR) error_exit(expr->location, "Pointers can only be cast to other pointer types.");
                    break;

                case TYPE_ARRAY:
                case TYPE_ID:
                case TYPE_FUNCTION: // You actually might want to be able to cast a function pointer.
                    error_exit(expr->location, "You can only cast to a primitive type or a pointer.");
                    break;
            }
            symbol_table_resolve_type(table, &expr->data.typecast.cast_to);
            return &expr->data.typecast.cast_to;
        } break;

        case EXPR_ACCESS_MEMBER: {
            Type *type = symbol_table_check_expr(table, expr->data.access_member.operand, is_rval, is_comptime);
            Type *sub_type = type;
            while (sub_type->type == TYPE_PTR || sub_type->type == TYPE_PTR_NULLABLE || sub_type->type == TYPE_ARRAY) {
                sub_type = sub_type->data.sub_type;
            }

            if (sub_type->type == TYPE_PRIMITIVE || sub_type->type == TYPE_FUNCTION) error_exit(expr->location, "Only complex types have members.");
            Declaration *decl = sub_type->data.id.type_declaration;
            switch (decl->type) {
                case DECLARATION_VAR:
                    assert(false); // Guaranteed not to be a var.
                    break;
                
                case DECLARATION_ENUM:
                    error_exit(expr->location, "Enum types do not have members.");
                    break;
                
                case DECLARATION_STRUCT:
                case DECLARATION_UNION:
                    for (int i = 0; i < decl->data.struct_union.member_count; i++) {
                        if (decl->data.struct_union.members[i].id.idx == expr->data.access_member.member.idx) {
                            if (!is_rval) *is_rval = type->type == TYPE_PTR || type->type == TYPE_PTR_NULLABLE || type->type == TYPE_ARRAY;
                            return type;
                        }
                    } 
                    error_exit(expr->location, "This complex type does not have a member with this name.");
                    break;

                case DECLARATION_SUM: 
                    error_exit(expr->location, "We haven't decided what to do if you try to directly access the member of a sum type yet.");
                    break;
            }
        } break;
        
        case EXPR_ACCESS_ARRAY: {
            bool rval_discard;
            Type *type = symbol_table_check_expr(table, expr->data.access_array.operand, &rval_discard, is_comptime);
            if (type->type != TYPE_ARRAY) error_exit(expr->location, "The operand of this array access is not an array.");
            *is_rval = true;
            return type->data.sub_type; 
        } break;

        default: 
            error_exit(expr->location, "Typechecking this kind of expression hasn't been implemented yet.");
            break;
    }
    assert(false);
}

// Inserts pointer to declaration to this type if it is an id.
// Only call this on types that were generated by the parser, not types that were inferred.
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
                case STATEMENT_DECLARATION: 
                    assert(false);
                    break;

                case STATEMENT_INCREMENT: {
                    bool is_rval;
                    Type *type = symbol_table_check_expr(table, &scope->data.statement.data.increment, &is_rval, false);
                    if (!is_rval) error_exit(scope->location, "Only rvals can be incremented.");
                    if (type->type != TYPE_PRIMITIVE || type->data.primitive < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MIN < type->data.primitive) {
                        error_exit(scope->location, "An incremented variable must be of an integer numeric type.");
                    }
                } break;

                default: assert(false);
            }
        } break;

        default: 
            error_exit(scope->location, "Typechecking this kind of scope hasn't been implemented yet.");
    }
}

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
        symbol_table_declaration_init(&table, decl);
    }

    /*
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++)
    for (int j = 0; j < table.nodes[i].declaration_count; j++) {
        Declaration *decl = table.nodes[i].declarations[j];
        if (decl->type != DECLARATION_VAR) continue;
        decl->state = DECLARATION_STATE_INITIALIZING;
        bool rval_discard;
        switch (decl->data.var.type) {
            case DECLARATION_VAR_CONSTANT: {
                Type *value_type = symbol_table_check_expr(&table, &decl->data.var.data.constant.value, &rval_discard, true);
                if (decl->data.var.data.constant.type_exists) {
                    if (!type_equal(value_type, &decl->data.var.data.constant.type)) { 
                        error_exit(decl->location, "The type of this constant and its assigned expression are not the same.");
                    }
                } else {
                    decl->data.var.constant.type = type_clone(value_type);
                }
            } break;
            
            case DECLARATION_VAR_MUTABLE: {
                if (&decl->data.var.data.mutable.value_exists) {
                    Type *value_type = symbol_table_check_declaration(table, &decl->data.var.data.mutable.value, &rval_discard, true);
                    if (!type_equal(value_type, &decl->data.var.data.mutable.type)) {
                        error_exit(decl->location, "The type of this variable and its assigned expression are not the same.");
                    }
                }
            } break;
        }
        symbol_table_check_expr(table, &decl->data.expr, rval_discard, 
    }
    */

    symbol_table_free(&table);
}
