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

void symbol_table_new(SymbolTable *table, SymbolTable *previous) {
    memset(table->nodes, 0, sizeof(SymbolNode) * SYMBOL_TABLE_NODE_COUNT);
    table->previous = previous;
}

void symbol_table_free_head(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++) {
        free(table->nodes[i].symbols);
    }
}

bool symbol_table_has(const SymbolTable *table, StringId id) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    while (table) { 
        for (int i = 0; i < table->nodes[idx].count; i++) {
            if (table->nodes[idx].symbols[i].id.idx == id.idx) return true;
        }
        table = table->previous;
    }
    return false;
}

bool symbol_table_get(const SymbolTable *table, StringId id, Symbol *symbol) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    while (table) {
        for (int i = 0; i < table->nodes[idx].count; i++) {
            if (table->nodes[idx].symbols[i].id.idx != id.idx) continue;
            *symbol = table->nodes[idx].symbols[i];
            return true; 
        }
        table = table->previous;
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
    
    int i = id.idx % SYMBOL_TABLE_NODE_COUNT;
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

TokenType symbol_table_check_expr(SymbolTable *table, Expr *expr) {
    switch (expr->type) {
        case EXPR_PAREN:
            return symbol_table_check_expr(table, expr->data.parenthesized);
        case EXPR_UNARY: {
            TokenType type = symbol_table_check_expr(table, expr->data.unary.operand);
            switch (expr->data.unary.type) {
                case EXPR_UNARY_LOGICAL_NOT:
                    if (type != TOKEN_KEYWORD_TYPE_BOOL) {
                        error_exit(expr->location, "The operand of a unary logical not expression is expected to have a boolean type.");
                    }
                    return TOKEN_KEYWORD_TYPE_BOOL;
                case EXPR_UNARY_BITWISE_NOT:
                    if (type < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < type) {
                        error_exit(expr->location, "The operand of a bitwise not expression is expected to have a numeric integer type.");            
                    }
                    return type;
                case EXPR_UNARY_NEGATE:
                    if (type < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < type) {
                        error_exit(expr->location, "The operand of a negation expression is expected to have a numeric type.");
                    }
                    return type;

                default: // ignoring ptr stuff for now.
                    assert(false);
            }
        }

        case EXPR_BINARY: {
            TokenType lhs = symbol_table_check_expr(table, expr->data.binary.lhs);
            TokenType rhs = symbol_table_check_expr(table, expr->data.binary.rhs);
            switch (expr->data.binary.operator) {
                case TOKEN_OP_LOGICAL_AND:
                case TOKEN_OP_LOGICAL_OR:
                    if (lhs != TOKEN_KEYWORD_TYPE_BOOL || rhs != TOKEN_KEYWORD_TYPE_BOOL) {
                        error_exit(expr->location, "The operands of a logical operator are both expected to have a boolean type.");
                    }
                    return TOKEN_KEYWORD_TYPE_BOOL;

                case TOKEN_OP_BITWISE_OR:
                case TOKEN_OP_BITWISE_AND:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of a bitwise operator are both expected to have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < lhs) {
                        error_exit(expr->location, "The operands of a bitwise operator must be of an unsigned integer type.");
                    }
                    return lhs;
                
                case TOKEN_OP_EQ:
                case TOKEN_OP_NE:
                    return TOKEN_KEYWORD_TYPE_BOOL;

                case TOKEN_OP_LT:
                case TOKEN_OP_GT:
                case TOKEN_OP_LE:
                case TOKEN_OP_GE:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of a comparison operator must both have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < lhs) {
                        error_exit(expr->location, "The operands of a comparison operator must both be of a numeric type.");
                    }
                    return TOKEN_KEYWORD_TYPE_BOOL;
                
                case TOKEN_OP_SHIFT_LEFT:
                case TOKEN_OP_SHIFT_RIGHT:
                    if (lhs < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < lhs) {
                        error_exit(expr->location, "The left operand of a shift operator must be of an unsigned integer type."); 
                    } else if (rhs < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < rhs) {
                        error_exit(expr->location, "The right operand of a shift operator must be of an unsigned integer type.");
                    }
                    return lhs;

                case TOKEN_OP_PLUS:
                case TOKEN_OP_MINUS:
                case TOKEN_OP_MULTIPLY:
                case TOKEN_OP_DIVIDE:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of an algebraic expression must both have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < lhs) {
                        error_exit(expr->location, "The operands of an algebraic experssion must be of a numeric type.");
                    }
                    return lhs;
                
                case TOKEN_OP_MODULO:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of a modulo expression must have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < lhs) {
                        error_exit(expr->location, "The operands of a modulo expression must be of an integer type.");
                    }
                    return lhs;
                
                default: 
                    assert(false);

            }
        }
        
        case EXPR_ID: {
            Symbol symbol;
            if (!symbol_table_get(table, expr->data.id, &symbol)) {
                error_exit(expr->location, "This identifier has not yet been declared.");
            }
            return symbol.type;
        }
        
        default: assert(false);
            
    }
}

void symbol_table_check_statement(SymbolTable *table, Statement *statement) {
    switch (statement->type) {
        case STATEMENT_VAR_DECLARE:
            if (!symbol_table_add_var(table, statement->data.var_declare.id, &statement->data.var_declare.type)) {
                error_exit(statement->location, "This local variable shadows another local variable in the same scope.");
            }
            break;

        case STATEMENT_INCREMENT: {
            TokenType type = symbol_table_check_expr(table, &statement->data.increment);
            if (type < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < type) {
                error_exit(statement->location, "Incremented variables must be of an integer numeric type.");
            }
        } break;
        
        case STATEMENT_DEINCREMENT: {
            TokenType type = symbol_table_check_expr(table, &statement->data.deincrement);
            if (type < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < type) {
                error_exit(statement->location, "Deincremented variables must be of an integer numeric type.");
            }
        } break;

        default: assert(false);
    }
}

void symbol_table_check_scope(SymbolTable *table, Scope *scope) {
    switch (scope->type) {
        case SCOPE_BLOCK: {
            SymbolTable table_new;
            symbol_table_new(&table_new, table);
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                symbol_table_check_scope(&table_new, scope->data.block.scopes + i);
            }
            symbol_table_free_head(&table_new);
        } break;
       
        case SCOPE_STATEMENT:
            symbol_table_check_statement(table, &scope->data.statement);
            break;
        
        case SCOPE_CONDITIONAL:
            if (symbol_table_check_expr(table, &scope->data.conditional.condition) != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.conditional.condition.location, "The expression of a while loop is expected to have a boolean type.");
            }
            symbol_table_check_scope(table, scope->data.conditional.scope_if);
            if (scope->data.conditional.scope_else) symbol_table_check_scope(table, scope->data.conditional.scope_else);
            break;

        case SCOPE_LOOP_FOR: {
            SymbolTable table_new;
            symbol_table_new(&table_new, table); 
            symbol_table_check_statement(&table_new, &scope->data.loop_for.init);
            if (symbol_table_check_expr(&table_new, &scope->data.loop_for.expr) != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.loop_for.expr.location, "The expression of a for loop is expected to have a boolean type.");
            }
            symbol_table_check_statement(&table_new, &scope->data.loop_for.step);
            symbol_table_check_scope(&table_new, scope->data.loop_for.scope);
            symbol_table_free_head(&table_new);
        } break;
        
        default: assert(false);
    }
}

void symbol_table_check_declaration(Declaration *decl) {
    switch (decl->type) {
        case DECLARATION_FUNCTION: {
            SymbolTable table;
            symbol_table_new(&table, NULL);
            for (int i = 0; i < decl->data.d_function.parameter_count; i++) {
                FunctionParameter param = decl->data.d_function.parameters[i];
                if (!symbol_table_add_var(&table, param.id, &param.type)) {
                    error_exit(param.type.location, "This function parameter's name shadows another.");
                }
            }
            symbol_table_check_scope(&table, &decl->data.d_function.scope);
            symbol_table_free_head(&table);
        } break;

        default: assert(false);
    }
}
