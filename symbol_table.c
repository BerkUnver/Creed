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

static Declaration *declarations = NULL;
static int declaration_count = 0;
static int declaration_count_alloc = 0;

#define SOURCE_FILE_TABLE_NODE_COUNT 512
typedef struct SourceFileTableNode {
    SourceFile *files;
    int file_count;
    int file_count_alloc;
} SourceFileTableNode;

static SourceFileTableNode source_file_table[SOURCE_FILE_TABLE_NODE_COUNT]; // Default allocates to all 0 so it is okay.

void source_file_add(StringId path) {
    // Check if source file was already parsed. If so, ignore it.
    int source_file_idx = path.idx % SOURCE_FILE_TABLE_NODE_COUNT;
    SourceFileTableNode *node = source_file_table + source_file_idx;
    for (int i = 0; i < node->file_count; i++) {
        if (node->files[i].id.idx == path.idx) return;
    }

    Lexer lexer = lexer_new(string_cache_get(path));

    StringId *imports = NULL;
    int import_count = 0;
    int import_count_alloc = 0;

    // Scan imports.
    while (lexer_token_peek(&lexer).type == TOKEN_KEYWORD_IMPORT) {
        Token token_import = lexer_token_get(&lexer);
        Token token_file = lexer_token_get(&lexer);
        if (token_file.type != TOKEN_LITERAL || token_file.data.literal.type != LITERAL_STRING) {
            error_exit(location_expand(token_import.location, token_file.location), "Expected a literal string as the name of an import.");
        }

        import_count++;
        if (import_count > import_count_alloc) {
            if (import_count_alloc == 0) import_count_alloc = 2;
            else import_count_alloc *= 2;
            imports = realloc(imports, sizeof(StringId) * import_count);
        }
        imports[import_count - 1] = token_file.data.id;
    }

    SourceFile source_file = {
        .id = path,
        .imports = imports,
        .import_count = import_count,
        .nodes = {{ 0 }}
    };

    // Add all declarations to the declaration list and add all declaration ids to the source file. 
    while (lexer_token_peek(&lexer).type != TOKEN_EOF) {
        Declaration decl = declaration_parse(&lexer);
        int decl_idx = decl.id.idx % SOURCE_FILE_NODE_COUNT;
        SourceFileNode *file_node = source_file.nodes + decl_idx;
        for (int i = 0; i < file_node->member_count; i++) {
            if (decl.id.idx != file_node->members[i].id.idx) continue;
            error_exit(decl.location, "A declaration with this name is already defined in the same file.");
        }
        file_node->member_count++;
        if (file_node->member_count > file_node->member_count_alloc) {
            if (file_node->member_count_alloc == 0) file_node->member_count_alloc = 2;
            else file_node->member_count_alloc *= 2;
            file_node->members = realloc(file_node->members, sizeof(SourceFileMember) * file_node->member_count_alloc);
        }

        declaration_count++;
        if (declaration_count > declaration_count_alloc) {
            if (declaration_count_alloc == 0) declaration_count_alloc = 512;
            else declaration_count_alloc = (int) ((float) declaration_count_alloc * 1.5f);
            declarations = realloc(declarations, sizeof(Declaration) * declaration_count_alloc);
        }
        int declaration_idx = declaration_count - 1;
        declarations[declaration_idx] = decl;
        file_node->members[file_node->member_count - 1] = (SourceFileMember) {.id = decl.id, .idx = declaration_idx}; 
    }

    lexer_free(&lexer); 
    // Add the source file to the source file table.
    node->file_count++;
    if (node->file_count > node->file_count_alloc) {
        if (node->file_count_alloc == 0) node->file_count_alloc = 2;
        else node->file_count_alloc *= 2;
        node->files = realloc(node->files, node->file_count_alloc * sizeof(SourceFile));
    }
    node->files[node->file_count - 1] = source_file;

    // Recusively parse imports (Has to happen after the source file is added to the source file table so it isn't parsed twice.)
    for (int i = 0; i < source_file.import_count; i++) {
        source_file_add(source_file.imports[i]);
    }
}

SourceFile *source_file_table_get(StringId path) {
    SourceFileTableNode *node = source_file_table + path.idx % SOURCE_FILE_NODE_COUNT;
    for (int i = 0; i < node->file_count; i++) {
        SourceFile *file = node->files + i;
        if (file->id.idx == path.idx) return file;
    }
    return NULL;
}

void source_file_print(SourceFile *file) {
    for (int i = 0; i < file->import_count; i++) {
        printf("%s %s\n", string_keywords[TOKEN_KEYWORD_IMPORT - TOKEN_KEYWORD_MIN], string_cache_get(file->imports[i]));
    }
    for (int i = 0; i < SOURCE_FILE_NODE_COUNT; i++) {
        for (int j = 0; j < file->nodes[i].member_count; j++) {
            putchar('\n');
            declaration_print(declarations + file->nodes[i].members[j].idx);
        }
    }
}

static int source_file_get_declaration_idx(SourceFile *source_file, StringId declaration_id) {
    SourceFileNode *node = source_file->nodes + declaration_id.idx % SOURCE_FILE_NODE_COUNT;
    for (int i = 0; i < node->member_count; i++) {
        if (node->members[i].id.idx == declaration_id.idx) return node->members[i].idx;
    }
    return -1;
}

static int source_file_table_get_declaration_idx(SourceFile *source_file, StringId declaration_id) {
    // Try to find declaration in file
    int decl_local = source_file_get_declaration_idx(source_file, declaration_id);
    if (decl_local >= 0) return decl_local;

    // Try to find declaration in imports
    for (int i = source_file->import_count - 1; i >= 0; i--) { // search imports from last to first
        SourceFile *import = source_file_table_get(source_file->imports[i]);
        assert(import);    
        int decl_import = source_file_get_declaration_idx(import, declaration_id);
        if (decl_import >= 0) return decl_import; 
    }
    return -1;
}

void source_file_table_init(StringId main_path) {
    source_file_add(main_path);
    for (int i = 0; i < SOURCE_FILE_TABLE_NODE_COUNT; i++) {
        SourceFileTableNode *table_node = source_file_table + i;
        for (int i = 0; i < table_node->file_count; i++) {
            SourceFile *file = table_node->files + i;
            for (int i = 0; i < SOURCE_FILE_NODE_COUNT; i++) {
                SourceFileNode *file_node = file->nodes + i;
                for (int i = 0; i < file_node->member_count; i++) {
                    Declaration *decl = declarations + file_node->members[i].idx;
                    if (decl->type != DECLARATION_FUNCTION) continue;
                    SymbolTable table = symbol_table_new(NULL);

                    for (int param_idx = 0; param_idx < decl->data.d_function.parameter_count; param_idx++) {
                        FunctionParameter *param = decl->data.d_function.parameters + param_idx;
                        if (!symbol_table_add_var(&table, file, param->id, &param->type)) {
                            error_exit(decl->location, "A function's parameter names must be unique.");
                        }
                    }
                    symbol_table_check_scope(&table, file, &decl->data.d_function.scope);
                    symbol_table_free_head(&table);
                }
            }
        }
    }
}

void source_file_table_free() {
    for (int i = 0; i < declaration_count; i++) {
        declaration_free(declarations + i);
    }
    free(declarations);
    for (int table_idx = 0; table_idx < SOURCE_FILE_TABLE_NODE_COUNT; table_idx++) {
        for (int file_idx = 0; file_idx < source_file_table[table_idx].file_count; file_idx++) {
            SourceFile *file = source_file_table[table_idx].files + file_idx;
            for (int decl_node_idx = 0; decl_node_idx < SOURCE_FILE_NODE_COUNT; decl_node_idx++) {
                free(file->nodes[decl_node_idx].members);
            }
            free(file->imports);
        }
        free(source_file_table[table_idx].files);
    }
}

bool symbol_type_equal(SymbolType lhs, SymbolType rhs) {
    return (lhs.is_primitive && rhs.is_primitive && lhs.data.primitive == rhs.data.primitive)
        || (!lhs.is_primitive && !rhs.is_primitive && lhs.data.declaration_idx == rhs.data.declaration_idx);
}

SymbolTable symbol_table_new(SymbolTable *previous) {
    SymbolTable table;
    memset(table.nodes, 0, sizeof(SymbolNode) * SYMBOL_TABLE_NODE_COUNT);
    table.previous = previous;
    return table;
}

void symbol_table_free_head(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++) {
        free(table->nodes[i].symbols);
    }
}

bool symbol_table_get(const SymbolTable *table, StringId id, Symbol *out) {
    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;
    while (table) {
        for (int i = 0; i < table->nodes[idx].symbol_count; i++) {
            if (table->nodes[idx].symbols[i].id.idx != id.idx) continue;
            *out = table->nodes[idx].symbols[i];
            return true; 
        }
        table = table->previous;
    }
    return false;
}

bool symbol_table_add_var(SymbolTable *table, SourceFile *file, StringId id, Type *type) {

    int idx = id.idx % SYMBOL_TABLE_NODE_COUNT;

    // Check to see if it is already in the table
    for (int i = 0; i < table->nodes[idx].symbol_count; i++) {
        if (table->nodes[idx].symbols[i].id.idx == id.idx) return false;
    }
    
    SymbolType symbol_type; // Get the type.
    switch (type->type) {
        case TYPE_PRIMITIVE:
            symbol_type.is_primitive = true;
            symbol_type.data.primitive = type->data.primitive;
            break;

        case TYPE_ID: {
            // If we eventually allow local types this will have to change.
            int decl_idx = source_file_table_get_declaration_idx(file, type->data.id);
            if (decl_idx < 0) {
                error_exit(type->location, "This is not the name of a type currently in scope.");
            }
            Declaration *decl = declarations + decl_idx;
            if (decl->type < DECLARATION_TYPE_MIN || DECLARATION_TYPE_MAX < decl->type) {
                error_exit(type->location, "This not the name of a type.");
            }
            symbol_type.is_primitive = false;
            symbol_type.data.declaration_idx = decl_idx;
        } break;

        case TYPE_PTR: // TODO: handle these.
        case TYPE_PTR_NULLABLE:
        case TYPE_ARRAY:
            assert(false);
    }

    Symbol symbol = {
        .id = id,
        .type = symbol_type
    };
    
    if (table->nodes[idx].symbol_count == 0) {
        int count_alloc = 2;
        Symbol *symbols = malloc(sizeof(Symbol) * count_alloc);
        symbols[0] = symbol;
        table->nodes[idx] = (SymbolNode) {
            .symbol_count = 1,
            .symbol_count_alloc = count_alloc,
            .symbols = symbols
        };
    } else {
        SymbolNode node = table->nodes[idx];
        node.symbol_count++;
        if (node.symbol_count > node.symbol_count_alloc) {
            node.symbol_count_alloc *= 2;
            node.symbols = realloc(node.symbols, sizeof(Symbol) * node.symbol_count_alloc);
        }
        node.symbols[node.symbol_count - 1] = symbol;
        table->nodes[idx] = node;
    }
    return true;
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
            Declaration *decl = declarations + type.data.declaration_idx;
            assert(DECLARATION_TYPE_MIN <= decl->type && decl->type <= DECLARATION_TYPE_MAX);
            
            // Find the member of the type with this name
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
            return symbol.type;
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

void symbol_table_check_statement(SymbolTable *table, SourceFile *file, Statement *statement) {
    switch (statement->type) {
        case STATEMENT_VAR_DECLARE: {
            if (!symbol_table_add_var(table, file, statement->data.var_declare.id, &statement->data.var_declare.type)) {
                error_exit(statement->location, "A variable with this name already exists in this scope.");
            }
            Symbol symbol;
            assert(symbol_table_get(table, statement->data.var_declare.id, &symbol));

            if (statement->data.var_declare.has_assign) {
                SymbolType expr_type = symbol_table_check_expr(table, &statement->data.var_declare.assign);
                if (!symbol_type_equal(symbol.type, expr_type)) {
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

void symbol_table_check_scope(SymbolTable *table, SourceFile *file, Scope *scope) {
    switch (scope->type) {
        case SCOPE_BLOCK: {
            SymbolTable table_new = symbol_table_new(table);
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                symbol_table_check_scope(&table_new, file, scope->data.block.scopes + i);
            }
            symbol_table_free_head(&table_new);
        } break;
       
        case SCOPE_STATEMENT:
            symbol_table_check_statement(table, file, &scope->data.statement);
            break;
        
        case SCOPE_CONDITIONAL: {
            SymbolType type = symbol_table_check_expr(table, &scope->data.conditional.condition);
            if (!type.is_primitive || type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.conditional.condition.location, "The expression of a while loop is expected to have a boolean type.");
            }
            symbol_table_check_scope(table, file, scope->data.conditional.scope_if);
            if (scope->data.conditional.scope_else) symbol_table_check_scope(table, file, scope->data.conditional.scope_else);
        } break;

        case SCOPE_LOOP_FOR: {
            SymbolTable table_new = symbol_table_new(table); 
            symbol_table_check_statement(&table_new, file, &scope->data.loop_for.init);
            SymbolType type = symbol_table_check_expr(&table_new, &scope->data.loop_for.expr);
            if (!type.is_primitive || type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.loop_for.expr.location, "The expression of a for loop is expected to have a boolean type.");
            }
            symbol_table_check_statement(&table_new, file, &scope->data.loop_for.step);
            symbol_table_check_scope(&table_new, file, scope->data.loop_for.scope);
            symbol_table_free_head(&table_new);
        } break;
        
        default: assert(false);
    }
}
