#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "prelude.h"
#include "string_cache.h"
#include "symbol_table.h"
#include "token.h"

bool symbol_type_equal(SymbolType lhs, SymbolType rhs) {
    return (lhs.is_primitive && rhs.is_primitive && lhs.data.primitive == rhs.data.primitive)
        || (!lhs.is_primitive && !rhs.is_primitive && lhs.data.id.idx == rhs.data.id.idx);
}

SymbolTable symbol_table_new(SymbolTable *previous) {
    SymbolTable table;
    memset(table.nodes, 0, sizeof(SymbolNode) * SYMBOL_TABLE_NODE_COUNT);
    table.previous = previous;
    return table;
}

void symbol_table_free_head(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++) {
        for (int symbol_idx = 0; symbol_idx < table->nodes[i].symbol_count; symbol_idx++) {
            Symbol *symbol = table->nodes[i].symbols + symbol_idx;
            if (symbol->type == SYMBOL_DECLARATION) {
                declaration_free(&symbol->data.declaration); 
            }
        }
        free(table->nodes[i].symbols);
    }
}

bool symbol_table_has(const SymbolTable *table, StringId id) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    while (table) { 
        for (int i = 0; i < table->nodes[idx].symbol_count; i++) {
            if (table->nodes[idx].symbols[i].id.idx == id.idx) return true;
        }
        table = table->previous;
    }
    return false;
}

bool symbol_table_get(const SymbolTable *table, StringId id, Symbol *symbol) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    while (table) {
        for (int i = 0; i < table->nodes[idx].symbol_count; i++) {
            if (table->nodes[idx].symbols[i].id.idx != id.idx) continue;
            *symbol = table->nodes[idx].symbols[i];
            return true; 
        }
        table = table->previous;
    }
    return false;
}

bool symbol_table_add_symbol(SymbolTable *table, Symbol symbol) {
    if (symbol_table_has(table, symbol.id)) return false;
    
    int i = symbol.id.idx % SYMBOL_TABLE_NODE_COUNT;
    if (table->nodes[i].symbol_count == 0) {
        int count_alloc = 2;
        Symbol *symbols = malloc(sizeof(Symbol) * count_alloc);
        symbols[0] = symbol;
        table->nodes[i] = (SymbolNode) {
            .symbol_count = 1,
            .symbol_count_alloc = count_alloc,
            .symbols = symbols
        };
    } else {
        SymbolNode node = table->nodes[i];
        node.symbol_count++;
        if (node.symbol_count > node.symbol_count_alloc) {
            node.symbol_count_alloc *= 2;
            node.symbols = realloc(node.symbols, sizeof(Symbol) * node.symbol_count_alloc);
        }
        node.symbols[node.symbol_count - 1] = symbol;
        table->nodes[i] = node;
    }
    return true;
}

bool symbol_table_add_var(SymbolTable *table, StringId id, Type *type) {
    SymbolType symbol_type;
    switch (type->type) {
        case TYPE_PRIMITIVE:
            symbol_type.is_primitive = true;
            symbol_type.data.primitive = type->data.primitive;
            break;

        case TYPE_ID: {
            Symbol symbol_type_decl;
            if (!symbol_table_get(table, type->data.id, &symbol_type_decl)) {
                error_exit(type->location, "This type does not exist.");
            }
            if (symbol_type_decl.type != SYMBOL_DECLARATION 
                || symbol_type_decl.data.declaration.type < DECLARATION_TYPE_MIN || DECLARATION_TYPE_MAX < symbol_type_decl.data.declaration.type) {
                error_exit(type->location, "This is not the name of a type.");
            }
            symbol_type.is_primitive = false;
            symbol_type.data.id = type->data.id;
        } break;

        case TYPE_PTR: // TODO: handle these.
        case TYPE_PTR_NULLABLE:
        case TYPE_ARRAY:
            assert(false);
    }

    Symbol symbol = {
        .id = id,
        .type = SYMBOL_VAR, 
        .data.var_type = symbol_type
    };
    return symbol_table_add_symbol(table, symbol); 
}

SymbolType symbol_table_check_expr(SymbolTable *table, Expr *expr) {
    switch (expr->type) {
        case EXPR_PAREN:
            return symbol_table_check_expr(table, expr->data.parenthesized);
        case EXPR_UNARY: {
            SymbolType symbol_type = symbol_table_check_expr(table, expr->data.unary.operand);
            if (!symbol_type.is_primitive) error_exit(expr->location, "The operand of a unary expression must be a primitive type.");
            TokenType type = symbol_type.data.primitive;
            switch (expr->data.unary.type) {
                case EXPR_UNARY_LOGICAL_NOT:
                    if (type != TOKEN_KEYWORD_TYPE_BOOL) {
                        error_exit(expr->location, "The operand of a unary logical not expression is expected to have a boolean type.");
                    }
                    return symbol_type;
                case EXPR_UNARY_BITWISE_NOT:
                    if (type < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < type) {
                        error_exit(expr->location, "The operand of a bitwise not expression is expected to have a numeric integer type.");            
                    }
                    return symbol_type;
                case EXPR_UNARY_NEGATE:
                    if (type < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < type) {
                        error_exit(expr->location, "The operand of a negation expression is expected to have a numeric type.");
                    }
                    return symbol_type;

                case EXPR_UNARY_REF: // ignoring ptr stuff for now.
                case EXPR_UNARY_DEREF:
                    assert(false);
            }
            assert(false);
        } break;

        case EXPR_BINARY: {
            SymbolType symbol_type_lhs = symbol_table_check_expr(table, expr->data.binary.lhs);
            SymbolType symbol_type_rhs = symbol_table_check_expr(table, expr->data.binary.rhs);
            
            if (!symbol_type_lhs.is_primitive || !symbol_type_rhs.is_primitive) error_exit(expr->location, "The operands of binary operators must be of a primitive type.");
            
            TokenType lhs = symbol_type_lhs.data.primitive;
            TokenType rhs = symbol_type_rhs.data.primitive;
            
            switch (expr->data.binary.operator) {
                case TOKEN_OP_LOGICAL_AND:
                case TOKEN_OP_LOGICAL_OR:
                    if (lhs != TOKEN_KEYWORD_TYPE_BOOL || rhs != TOKEN_KEYWORD_TYPE_BOOL) {
                        error_exit(expr->location, "The operands of a logical operator are both expected to have a boolean type.");
                    }
                    return symbol_type_lhs;

                case TOKEN_OP_BITWISE_OR:
                case TOKEN_OP_BITWISE_AND:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of a bitwise operator are both expected to have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < lhs) {
                        error_exit(expr->location, "The operands of a bitwise operator must be of an unsigned integer type.");
                    }
                    return symbol_type_lhs;
                
                case TOKEN_OP_EQ:
                case TOKEN_OP_NE:
                    if (lhs != rhs) error_exit(expr->location, "The operands of a comparison operator are expected to be of the same type.");
                    return (SymbolType) {.is_primitive = true, .data.primitive = TOKEN_KEYWORD_TYPE_BOOL};

                case TOKEN_OP_LT:
                case TOKEN_OP_GT:
                case TOKEN_OP_LE:
                case TOKEN_OP_GE:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of a comparison operator must both have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < lhs) {
                        error_exit(expr->location, "The operands of a comparison operator must both be of a numeric type.");
                    }
                    return (SymbolType) {.is_primitive = true, .data.primitive = TOKEN_KEYWORD_TYPE_BOOL};
                
                case TOKEN_OP_SHIFT_LEFT:
                case TOKEN_OP_SHIFT_RIGHT:
                    if (lhs < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < lhs) {
                        error_exit(expr->location, "The left operand of a shift operator must be of an unsigned integer type."); 
                    } else if (rhs < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < rhs) {
                        error_exit(expr->location, "The right operand of a shift operator must be of an unsigned integer type.");
                    }
                    return symbol_type_lhs;

                case TOKEN_OP_PLUS:
                case TOKEN_OP_MINUS:
                case TOKEN_OP_MULTIPLY:
                case TOKEN_OP_DIVIDE:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of an algebraic expression must both have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < lhs) {
                        error_exit(expr->location, "The operands of an algebraic experssion must be of a numeric type.");
                    }
                    return symbol_type_lhs;
                
                case TOKEN_OP_MODULO:
                    if (lhs != rhs) {
                        error_exit(expr->location, "The operands of a modulo expression must have the same type.");
                    } else if (lhs < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < lhs) {
                        error_exit(expr->location, "The operands of a modulo expression must be of an integer type.");
                    }
                    return symbol_type_lhs;
                
                default: 
                    assert(false);

            }
        } break;
 
        case EXPR_TYPECAST: 
            assert(false);

        case EXPR_ACCESS_MEMBER: {
            SymbolType type = symbol_table_check_expr(table, expr->data.access_member.operand);
            if (type.is_primitive) error_exit(expr->location, "The type of the operand of a member access expression must be either a struct or union.");
            Symbol symbol;
            assert(symbol_table_get(table, type.data.id, &symbol) && symbol.type == SYMBOL_DECLARATION);
            Declaration *decl = &symbol.data.declaration;
            assert(DECLARATION_TYPE_MIN <= decl->type && decl->type <= DECLARATION_TYPE_MAX);
            for (int i = 0; i < decl->data.d_struct_union.member_count; i++) {
                MemberStructUnion *member = decl->data.d_struct_union.members + i;
                if (expr->data.access_member.member.idx != member->id.idx) continue;
                assert(member->type.type == TYPE_PRIMITIVE); // TODO: temporary restriction: structs can only have members of primitive types.
                return (SymbolType) {.is_primitive = true, .data.primitive = member->type.data.primitive};
            }
            error_exit(expr->location, "This struct has no member with this name.");
        } break;
        
        case EXPR_ACCESS_ARRAY:
        case EXPR_FUNCTION_CALL:
            assert(false);
        
        case EXPR_ID: {
            Symbol symbol;
            if (!symbol_table_get(table, expr->data.id, &symbol)) {
                error_exit(expr->location, "This identifier has not yet been declared.");
            }
            if (symbol.type != SYMBOL_VAR) {
                error_exit(expr->location, "This symbol is not a variable name.");
            }
            return symbol.data.var_type;
        }
        
        case EXPR_LITERAL: {
            assert(expr->data.literal.type != LITERAL_STRING);
            TokenType type = expr->data.literal.type - LITERAL_CHAR + TOKEN_KEYWORD_TYPE_CHAR;
            return (SymbolType) {.is_primitive = true, .data.primitive = type};
        }

        case EXPR_LITERAL_BOOL:
            return (SymbolType) {.is_primitive = true, .data.primitive = TOKEN_KEYWORD_TYPE_BOOL};
            
    }
    assert(false);
}

void symbol_table_check_statement(SymbolTable *table, Statement *statement) {
    switch (statement->type) {
        case STATEMENT_VAR_DECLARE: {
            symbol_table_add_var(table, statement->data.var_declare.id, &statement->data.var_declare.type);
            Symbol symbol;
            assert(symbol_table_get(table, statement->data.var_declare.id, &symbol));
            assert(symbol.type == SYMBOL_VAR);

            if (statement->data.var_declare.has_assign) {
                SymbolType expr_type = symbol_table_check_expr(table, &statement->data.var_declare.assign);
                if (!symbol_type_equal(symbol.data.var_type, expr_type)) {
                    error_exit(statement->location, "The declared type of this variable is not the same as the type of its assignment.");
                }
            }
        } break;
        
        // We need a way to differenciate lvals and rvals here.
        case STATEMENT_INCREMENT: {
            SymbolType type = symbol_table_check_expr(table, &statement->data.increment);
            if (!type.is_primitive || type.data.primitive < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < type.data.primitive) {
                error_exit(statement->location, "Incremented variables must be of an integer numeric type.");
            }
        } break;
        
        case STATEMENT_DEINCREMENT: {
            SymbolType type = symbol_table_check_expr(table, &statement->data.deincrement);
            if (!type.is_primitive || type.data.primitive < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < type.data.primitive) {
                error_exit(statement->location, "Deincremented variables must be of an integer numeric type.");
            }
        } break;

        default: assert(false);
    }
}

void symbol_table_check_scope(SymbolTable *table, Scope *scope) {
    switch (scope->type) {
        case SCOPE_BLOCK: {
            SymbolTable table_new = symbol_table_new(table);
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                symbol_table_check_scope(&table_new, scope->data.block.scopes + i);
            }
            symbol_table_free_head(&table_new);
        } break;
       
        case SCOPE_STATEMENT:
            symbol_table_check_statement(table, &scope->data.statement);
            break;
        
        case SCOPE_CONDITIONAL: {
            SymbolType type = symbol_table_check_expr(table, &scope->data.conditional.condition);
            if (!type.is_primitive || type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.conditional.condition.location, "The expression of a while loop is expected to have a boolean type.");
            }
            symbol_table_check_scope(table, scope->data.conditional.scope_if);
            if (scope->data.conditional.scope_else) symbol_table_check_scope(table, scope->data.conditional.scope_else);
        } break;

        case SCOPE_LOOP_FOR: {
            SymbolTable table_new = symbol_table_new(table); 
            symbol_table_check_statement(&table_new, &scope->data.loop_for.init);
            SymbolType type = symbol_table_check_expr(&table_new, &scope->data.loop_for.expr);
            if (!type.is_primitive || type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.loop_for.expr.location, "The expression of a for loop is expected to have a boolean type.");
            }
            symbol_table_check_statement(&table_new, &scope->data.loop_for.step);
            symbol_table_check_scope(&table_new, scope->data.loop_for.scope);
            symbol_table_free_head(&table_new);
        } break;
        
        default: assert(false);
    }
}

void symbol_table_check_functions(SymbolTable *table) {
    for (int node_idx = 0; node_idx < SYMBOL_TABLE_NODE_COUNT; node_idx++) {
        for (int symbol_idx = 0; symbol_idx < table->nodes[node_idx].symbol_count; symbol_idx++) {
            Symbol *symbol = table->nodes[node_idx].symbols + symbol_idx;
            if (symbol->type != SYMBOL_DECLARATION) continue;
            Declaration *decl = &symbol->data.declaration;
            if (decl->type != DECLARATION_FUNCTION) continue;
            SymbolTable table_func = symbol_table_new(table);
            for (int param_idx = 0; param_idx < decl->data.d_function.parameter_count; param_idx++) {
                FunctionParameter *param = decl->data.d_function.parameters + param_idx;
                if (!symbol_table_add_var(&table_func, param->id, &param->type)) {
                    error_exit(decl->location, "A function's parameter names must be unique.");
                }
            }
            symbol_table_check_scope(&table_func, &decl->data.d_function.scope);
            symbol_table_free_head(&table_func);
        }
    }
}

void symbol_table_print(SymbolTable *table) {
    for (int node_idx = 0; node_idx < SYMBOL_TABLE_NODE_COUNT; node_idx++) {
        for (int symbol_idx = 0; symbol_idx < table->nodes[node_idx].symbol_count; symbol_idx++) {
            Symbol *symbol = table->nodes[node_idx].symbols + symbol_idx;
            assert(symbol->type == SYMBOL_DECLARATION);
            declaration_print(&symbol->data.declaration);
            putchar('\n');
        }
    }
}

SymbolTable symbol_table_from_file(const char *path) {
    SymbolTable table = symbol_table_new(NULL);
    
    Lexer lexer = lexer_new(path);
    
    while (lexer_token_peek(&lexer).type == TOKEN_KEYWORD_IMPORT) {
        Token token_import = lexer_token_get(&lexer);
        Token token_file = lexer_token_get(&lexer);
        if (token_file.type != TOKEN_LITERAL || token_file.data.literal.type != LITERAL_STRING) {
            error_exit(location_expand(token_import.location, token_file.location), "Expected a literal string as the name of an import.");
        }
        
         // Ignoring imports for now.
    }
 
    while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
        Declaration decl = declaration_parse(&lexer);
        Symbol symbol = {
            .id = decl.id,
            .type = SYMBOL_DECLARATION,
            .data.declaration = decl
        };
        if (!symbol_table_add_symbol(&table, symbol)) {
            error_exit(decl.location, "A declaration with this name already exists in this file.");
        }
    }
    lexer_free(&lexer);
    return table;
}
