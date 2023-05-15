#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"

void expr_result_free(ExprResult *result) {
    type_free(&result->type);
}

void symbol_table_new(SymbolTable *out, SymbolTable *previous) {
    memset(&out->nodes, 0, sizeof(SymbolTableNode) * SYMBOL_TABLE_NODE_COUNT);
    out->previous = previous;
}

void symbol_table_free(SymbolTable *table) {
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++) {
        free(table->nodes[i].declarations);
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
                symbol_table_resolve_type(table, &type->data.function.params[i].type);
            }
            symbol_table_resolve_type(table, type->data.function.result);
        } break;
    }
}

static void symbol_table_declaration_init(SymbolTable *table, Declaration *decl) {
    int decl_state = decl->state;
    decl->state = DECLARATION_STATE_INITIALIZING;

    switch (decl_state) {
        case DECLARATION_STATE_UNINITIALIZED:
            switch (decl->type) {
                case DECLARATION_VAR: {
                    switch (decl->data.var.type) {
                        case DECLARATION_VAR_CONSTANT: {
                            ExprResult result = symbol_table_check_expr(table, &decl->data.var.data.constant.value);
                            if (result.state != EXPR_RESULT_CONSTANT) error_exit(decl->location, "The value of a constant must itself be derivable from constants.");
                            
                            if (decl->data.var.data.constant.type_explicit) {
                                symbol_table_resolve_type(table, &decl->data.var.data.constant.type); 
                                if (!type_equal(&result.type, &decl->data.var.data.constant.type)) {
                                    error_exit(decl->location, "The type of this constant and its assigned expression are not the same.");
                                }
                                expr_result_free(&result);
                            } else {
                                decl->data.var.data.constant.type = result.type;
                                // Do not free the result here because its type is used here.
                            }
                        } break;

                        case DECLARATION_VAR_MUTABLE: {
                            symbol_table_resolve_type(table, &decl->data.var.data.mutable.type);
                            if (decl->data.var.data.mutable.value_exists) {
                                ExprResult result = symbol_table_check_expr(table, &decl->data.var.data.mutable.value);
                                if (!type_equal(&result.type, &decl->data.var.data.mutable.type)) {
                                    error_exit(decl->location, "The type of this variable and its assigned expression are not the same.");
                                }
                                expr_result_free(&result);
                            }
                        } break;

                    }
                } break;

                case DECLARATION_ENUM: {
                    error_exit(decl->location, "Enums are currently not supported.");
                } break;

                case DECLARATION_STRUCT:
                case DECLARATION_UNION: {

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
                } break;

                case DECLARATION_SUM: {
                    error_exit(decl->location, "Sum types are currently not supported.");
                } break;
            } break;

        case DECLARATION_STATE_INITIALIZING: 
            error_exit(decl->location, "This declaration contains a circular reference.");
            break;

        case DECLARATION_STATE_INITIALIZED:
            break;
    }
    decl->state = DECLARATION_STATE_INITIALIZED;
}

ExprResult symbol_table_check_expr(SymbolTable *table, Expr *expr) {
    switch (expr->type) {
        case EXPR_PAREN:
            return symbol_table_check_expr(table, expr->data.parenthesized);
        case EXPR_UNARY: {
            ExprResult result = symbol_table_check_expr(table, expr->data.unary.operand);
            if (result.type.type != TYPE_PRIMITIVE) {
                error_exit(expr->location, "The operand of a unary expression must have a primitive type.");
            }
            switch (expr->data.unary.type) {
                case EXPR_UNARY_LOGICAL_NOT:
                    if (result.type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) error_exit(expr->location, "The operand of a negation expression must have a boolean type.");
                    return result;
                
                case EXPR_UNARY_BITWISE_NOT:
                    if (result.type.data.primitive < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < result.type.data.primitive) {
                        error_exit(expr->location, "The operand of a bitwise not expression must have a unsigned integer type.");
                    }
                    return result;

                case EXPR_UNARY_REF:
                    if (result.state != EXPR_RESULT_LVAL) {
                        error_exit(expr->location, "The operand of a reference must be an lval.");
                    }

                    Type *sub_type = malloc(sizeof(Type));
                    *sub_type = result.type;

                    return (ExprResult) {
                        .type = (Type) {
                            .location = result.type.location,
                            .type = TYPE_PTR,
                            .data.sub_type = sub_type
                        },
                        .state = EXPR_RESULT_RVAL,
                    };
                
                case EXPR_UNARY_DEREF:
                    if (result.type.type != TYPE_PTR && result.type.type != TYPE_PTR_NULLABLE) { 
                        error_exit(expr->location, "The operand of a dereference must be a pointer.");
                    }
                    
                    ExprResult deref_result = { 
                        .type = type_clone(result.type.data.sub_type),
                        .state = result.state == EXPR_RESULT_CONSTANT ? EXPR_RESULT_CONSTANT : EXPR_RESULT_LVAL
                    };

                    expr_result_free(&result);
                    return deref_result;

                case EXPR_UNARY_NEGATE: // TODO: what is this operator?
                default: 
                    error_exit(expr->location, "Typechecking this unary operator is not implemented yet.");
            }
        } break;

        case EXPR_BINARY: { // unfinished
            ExprResult result_lhs = symbol_table_check_expr(table, expr->data.binary.lhs);
            ExprResult result_rhs = symbol_table_check_expr(table, expr->data.binary.rhs);
            
            if (result_lhs.type.type != TYPE_PRIMITIVE || result_rhs.type.type != TYPE_PRIMITIVE) {
                error_exit(expr->location, "The operands of a binary expression must both be of a primitive type.");
            }
            
            int state = result_lhs.state == EXPR_RESULT_CONSTANT && result_rhs.state == EXPR_RESULT_CONSTANT ? EXPR_RESULT_CONSTANT : EXPR_RESULT_RVAL;
            
            switch (expr->data.binary.operator) {
                case TOKEN_OP_LOGICAL_AND:
                case TOKEN_OP_LOGICAL_OR:
                    if (result_lhs.type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL || result_rhs.type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                        error_exit(expr->location, "The operands of a logical operator are both expected to have a boolean type.");
                    }

                    result_lhs.state = state;
                    expr_result_free(&result_rhs);
                    return result_lhs;
                
                case TOKEN_OP_GE:
                case TOKEN_OP_LE:
                case TOKEN_OP_GT:
                case TOKEN_OP_LT:
                    if (result_lhs.type.data.primitive != result_rhs.type.data.primitive) {
                        error_exit(expr->location, "The operands of a binary comparison operator must be the same type.");
                    }
                    if (result_lhs.type.data.primitive < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < result_lhs.type.data.primitive) {
                        error_exit(expr->location, "The operands of a binary comparison operator must be numeric types.");
                    }
                    
                    expr_result_free(&result_lhs);
                    expr_result_free(&result_rhs);

                    return (ExprResult) {
                        .state = state,
                        .type = (Type) {
                            .type = TYPE_PRIMITIVE,
                            .data.primitive = TOKEN_KEYWORD_TYPE_BOOL
                        }
                    };

                case TOKEN_OP_EQ:
                case TOKEN_OP_NE:
                    if (result_lhs.type.data.primitive != result_rhs.type.data.primitive) {
                        error_exit(expr->location, "The operands of an equality check must be of the same type.");
                    }
                    expr_result_free(&result_lhs);
                    expr_result_free(&result_rhs);
                    
                    return (ExprResult) {
                        .state = state,
                        .type = (Type) {
                            .type = TYPE_PRIMITIVE,
                            .data.primitive = TOKEN_KEYWORD_TYPE_BOOL
                        }
                    };

                case TOKEN_OP_SHIFT_LEFT:
                case TOKEN_OP_SHIFT_RIGHT:

                    if (result_lhs.type.data.primitive < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < result_lhs.type.data.primitive) {
                        error_exit(expr->location, "The left operand of a bit shift expression must be of an unsigned integer type.");
                    }
                    if (result_rhs.type.data.primitive < TOKEN_KEYWORD_TYPE_UINT_MIN || TOKEN_KEYWORD_TYPE_UINT_MAX < result_rhs.type.data.primitive) {
                        error_exit(expr->location, "The right operand of a bit shift expression must be of an unsigned integer type.");
                    }
                    
                    expr_result_free(&result_rhs);
                    result_lhs.state = state;
                    return result_lhs;

                case TOKEN_OP_MODULO:
                    if (result_lhs.type.data.primitive != result_rhs.type.data.primitive) {
                        error_exit(expr->location, "The operands of a modulo expression must be of the same type.");
                    }
                    if (result_lhs.type.data.primitive < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < result_lhs.type.data.primitive) {
                        error_exit(expr->location, "The operands of a modulo expression must of an integer type.");
                    }

                    expr_result_free(&result_rhs);
                    result_lhs.state = state;
                    return result_lhs;
                
                case TOKEN_OP_PLUS:
                case TOKEN_OP_MINUS:
                case TOKEN_OP_MULTIPLY:
                case TOKEN_OP_DIVIDE:
                    if (result_lhs.type.data.primitive != result_rhs.type.data.primitive) { 
                        error_exit(expr->location, "The operands of an arithmetic expression must be of the same type.");
                    }
                    if (result_lhs.type.data.primitive < TOKEN_KEYWORD_TYPE_NUMERIC_MIN || TOKEN_KEYWORD_TYPE_NUMERIC_MAX < result_lhs.type.data.primitive) {
                        error_exit(expr->location, "The operands of an arithmetic expression must be of a numeric type.");
                    }
                    expr_result_free(&result_rhs);
                    result_lhs.state = state;
                    return result_lhs;
                
                default: 
                    error_exit(expr->location, "Typechecking this binary operator is not implemented yet.");
                    assert(false);
            }
        } break;
        
        case EXPR_TYPECAST: {
            ExprResult result = symbol_table_check_expr(table, expr->data.typecast.operand);
            switch (result.type.type) {
                case TYPE_PRIMITIVE:
                    if (result.type.data.primitive == TOKEN_KEYWORD_TYPE_VOID) error_exit(expr->location, "You cannot cast a void expression.");
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
            ExprResult cast = {
                .type = type_clone(&expr->data.typecast.cast_to),
                .state = result.state == EXPR_RESULT_CONSTANT ? EXPR_RESULT_CONSTANT : EXPR_RESULT_RVAL,
            };
            expr_result_free(&result);
            return cast;
        } break;

        case EXPR_ACCESS_MEMBER: {
            ExprResult result = symbol_table_check_expr(table, expr->data.access_member.operand);
            Type *sub_type = &result.type;
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
                            int state;
                            if (result.state == EXPR_RESULT_CONSTANT) state = EXPR_RESULT_CONSTANT;
                            else if (result.state == EXPR_RESULT_LVAL
                                    || result.type.type == TYPE_PTR 
                                    || result.type.type == TYPE_PTR_NULLABLE 
                                    || result.type.type == TYPE_ARRAY) 
                                    state = EXPR_RESULT_LVAL;
                            else state = EXPR_RESULT_RVAL;

                            ExprResult member = {
                                .state = state,
                                .type = type_clone(&decl->data.struct_union.members[i].type)
                            };
                            expr_result_free(&result);
                            return member;
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
            error_exit(expr->location, "Array access expressions are currently not implemented.");
            /*
            ExprResult operand_result = symbol_table_check_expr(table, expr->data.access_array.operand);
            if (operand_result.type.type != TYPE_ARRAY) {
                error_exit(expr->location, "The operand of this array access is not an array.");
            }
            
            ExprResult item = {
                .type = type_clone(operand_result.type.data.sub_type),
                .state = operand_result.state == EXPR_RESULT_CONSTANT ? EXPR_RESULT_CONSTANT : EXPR_RESULT_LVAL
            };
            expr_result_free(&operand_result);
            return item;
            */
        } break;

        case EXPR_FUNCTION: {
            symbol_table_resolve_type(table, &expr->data.function.type);
            SymbolTable *table_global = table;
            while (table_global->previous) table_global = table_global->previous; // Fetch the global symbol table
           
            SymbolTable table_function;
            symbol_table_new(&table_function, table_global);
            // TODO: Add function parameters.
            symbol_table_check_scope(table, expr->data.function.scope, expr->data.function.type.data.function.result);
            return (ExprResult) {
                .state = EXPR_RESULT_CONSTANT,
                .type = type_clone(&expr->data.function.type)
            };
        } break;
        
        case EXPR_FUNCTION_CALL: {
            ExprResult function_result = symbol_table_check_expr(table, expr->data.function_call.function);
            if (function_result.type.type != TYPE_FUNCTION) {
                error_exit(expr->data.function_call.function->location, "This expression does not have a function type, so it cannot be called.");        
            }
            if (function_result.type.data.function.param_count != expr->data.function_call.param_count) {
                error_exit(expr->location, "The number of parameters in this function call and its type does not match.");
            }
            for (int i = 0; i < function_result.type.data.function.param_count; i++) {
                ExprResult param_result = symbol_table_check_expr(table, expr->data.function_call.params + i);
                if (!type_equal(&param_result.type, &function_result.type.data.function.params[i].type)) {
                    error_exit(expr->data.function_call.params[i].location, "The type of expression does not match the type of the function parameter.");
                }
            }
            Type return_type = type_clone(function_result.type.data.function.result);
            expr_result_free(&function_result); 
            // Technically instead of freeing this you could transfer the allocation of the function result type and free the parameters,
            // but I don't want to put that logic here.
            return (ExprResult) {
                .state = EXPR_RESULT_RVAL,
                .type = return_type
            };
        } break;
        
        case EXPR_ID: {
            Declaration *decl = symbol_table_get(table, expr->data.id);
            if (!decl) error_exit(expr->location, "This identifier does not exist in the current scope.");
            if (decl->type != DECLARATION_VAR) {
                error_exit(expr->location, "This identifier is the name of a type, not a variable.");
            }
            symbol_table_declaration_init(table, decl); // Make sure the declaration is initialized if this is in the global scope.
            
            switch (decl->data.var.type) {
                case DECLARATION_VAR_CONSTANT:
                    return (ExprResult) {
                        .state = EXPR_RESULT_CONSTANT,
                        .type = type_clone(&decl->data.var.data.constant.type)
                    };
                case DECLARATION_VAR_MUTABLE:
                    return (ExprResult) {
                        .state = EXPR_RESULT_LVAL,
                        .type = type_clone(&decl->data.var.data.mutable.type)
                    };
            }
        } break;
        
        case EXPR_LITERAL: {
            Type type;
            if (expr->data.literal.type == LITERAL_STRING) {
                Type *sub_type = malloc(sizeof(Type));
                *sub_type = (Type) {
                    .type = TYPE_PRIMITIVE,
                    .data.primitive = TOKEN_KEYWORD_TYPE_CHAR
                };
                type = (Type) { 
                    .type = TYPE_ARRAY,
                    .data.sub_type = sub_type
                };
            } else {
                type = (Type) {
                    .type = TYPE_PRIMITIVE,
                    .data.primitive = TOKEN_KEYWORD_TYPE_CHAR + expr->data.literal.type - LITERAL_CHAR 
                };
            }

            return (ExprResult) {
                .type = type,
                .state = EXPR_RESULT_CONSTANT
            };
        } break;
        
        case EXPR_LITERAL_BOOL: {
            return (ExprResult) {
                .type = (Type) {
                    .type = TYPE_PRIMITIVE,
                    .data.primitive = TOKEN_KEYWORD_TYPE_BOOL
                },
                .state = EXPR_RESULT_CONSTANT
            };
        } break;

        case EXPR_LITERAL_ARRAY: {
            // TODO: typecheck members and check to make sure member count is a constant.
            Type *sub_type = malloc(sizeof(Type));
            *sub_type = type_clone(&expr->data.literal_array.type);
            
            return (ExprResult) {
                .type = (Type) {
                    .type = TYPE_ARRAY,
                    .data.sub_type = sub_type
                },
                .state = EXPR_RESULT_CONSTANT
            };
        } break;
    }
    assert(false);
}


void symbol_table_check_statement(SymbolTable *table, Statement *statement, Type *return_type) {
    switch (statement->type) {
        case STATEMENT_DECLARATION: {
            Declaration *decl = &statement->data.declaration;
            if (!symbol_table_insert(table, decl)) {
                error_exit(decl->location, "A declaration with this name already exists in this scope.");
            }
            symbol_table_declaration_init(table, decl);
        } break;

        case STATEMENT_INCREMENT:
        case STATEMENT_DEINCREMENT: {
            Expr *increment = statement->type == STATEMENT_INCREMENT ? &statement->data.increment : &statement->data.deincrement;
            ExprResult result = symbol_table_check_expr(table, increment);
            if (result.state != EXPR_RESULT_LVAL) error_exit(statement->location, "Only lvals can be incremented.");
            if (result.type.type != TYPE_PRIMITIVE || result.type.data.primitive < TOKEN_KEYWORD_TYPE_INTEGER_MIN || TOKEN_KEYWORD_TYPE_INTEGER_MAX < result.type.data.primitive) {
                error_exit(statement->location, "An incremented variable must be of an integer numeric type.");
            }
            expr_result_free(&result);
        } break;

        case STATEMENT_ASSIGN: {
            ExprResult result = symbol_table_check_expr(table, &statement->data.assign.assignee);
            if (result.state != EXPR_RESULT_LVAL) error_exit(statement->location, "You can only assign to lvals.");
            ExprResult value_result = symbol_table_check_expr(table, &statement->data.assign.value);
            if (!type_equal(&result.type, &value_result.type)) {
                error_exit(statement->location, "The assignee and assigned value in an assignment statement must be of the same type.");
            }
            switch (statement->data.assign.type) {
                case TOKEN_ASSIGN: 
                    break;
                default:
                    error_exit(statement->location, "Typechecking this type of assignment statement is not yet implemented.");
                    break;
            }
            expr_result_free(&result);
            expr_result_free(&value_result);
        } break;

        case STATEMENT_EXPR: {
            ExprResult result = symbol_table_check_expr(table, &statement->data.expr);
            if (result.type.type != TYPE_PRIMITIVE || result.type.data.primitive != TOKEN_KEYWORD_TYPE_VOID) {
                error_exit(statement->location, "You cannot implicitly discard the value of an expression.");                        
            }
            expr_result_free(&result);
        } break;


        case STATEMENT_RETURN: {
            if (!statement->data.return_value.exists) {
                if (return_type->type != TYPE_PRIMITIVE || return_type->data.primitive != TOKEN_KEYWORD_TYPE_VOID) {
                    error_exit(statement->location, "Missing return value.");
                }
            } else {
                ExprResult result = symbol_table_check_expr(table, &statement->data.return_value.expr);
                if (!type_equal(&result.type, return_type)) {
                    error_exit(statement->location, "The return type of this statement does not match the return type of this function.");
                }
                expr_result_free(&result);
            }
        } break;

        default: 
            error_exit(statement->location, "Typechecking this kind of statement hasn't been implemented yet.");
            break;
    }
}
// Inserts pointer to declaration to this type if it is an id.
// Only call this on types that were generated by the parser, not types that were inferred.
void symbol_table_check_scope(SymbolTable *table, Scope *scope, Type *return_type) { 
    switch (scope->type) {
        case SCOPE_BLOCK: {
            SymbolTable table_scope;
            symbol_table_new(&table_scope, table);
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                symbol_table_check_scope(&table_scope, scope->data.block.scopes + i, return_type);
            }
            symbol_table_free(&table_scope);
        } break;

        case SCOPE_STATEMENT: {
            symbol_table_check_statement(table, &scope->data.statement, return_type);
        } break;
        
        case SCOPE_CONDITIONAL: {
            ExprResult result = symbol_table_check_expr(table, &scope->data.conditional.condition);
            if (result.type.type != TYPE_PRIMITIVE || result.type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.conditional.condition.location, "The type of the condition of an if statement is expected to be a boolean.");                
            }
            symbol_table_check_scope(table, scope->data.conditional.scope_if, return_type);
            if (scope->data.conditional.scope_else) symbol_table_check_scope(table, scope->data.conditional.scope_else, return_type);
        } break;



        case SCOPE_LOOP_FOR: {
            SymbolTable table_scope;
            symbol_table_new(&table_scope, table);
            symbol_table_check_statement(&table_scope, &scope->data.loop_for.init, return_type);
            ExprResult result = symbol_table_check_expr(&table_scope, &scope->data.loop_for.expr);
            if (result.type.type != TYPE_PRIMITIVE || result.type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.loop_for.expr.location, "The expression of a for loop is expected to be of a boolean type.");
            }
            expr_result_free(&result);
            symbol_table_check_statement(&table_scope, &scope->data.loop_for.step, return_type);
            symbol_table_check_scope(&table_scope, scope->data.loop_for.scope, return_type);
            symbol_table_free(&table_scope);
        } break;

        case SCOPE_LOOP_WHILE: {
            ExprResult result = symbol_table_check_expr(table, &scope->data.loop_while.expr);
            if (result.type.type != TYPE_PRIMITIVE || result.type.data.primitive != TOKEN_KEYWORD_TYPE_BOOL) {
                error_exit(scope->data.loop_while.expr.location, "The expression of a while loop is expected to be of a boolean type.");
            }
            expr_result_free(&result);
            symbol_table_check_scope(table, scope->data.loop_while.scope, return_type);
        } break;
        
        default:
            error_exit(scope->location, "Typechecking this kind of scope hasn't been implemented yet.");
            break;
    }
}

void typecheck(SourceFile *file) {
    SymbolTable table;
    symbol_table_new(&table, NULL);
   
    for (int i = 0; i < file->declaration_count; i++) {
        if (symbol_table_insert(&table, file->declarations + i)) continue;
        error_exit(file->declarations[i].location, "This declaration has a duplicate name.");
    }
    
    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++)
    for (int j = 0; j < table.nodes[i].declaration_count; j++) {
        Declaration *decl = table.nodes[i].declarations[j];
        if (decl->type == DECLARATION_VAR) continue;
        symbol_table_declaration_init(&table, decl);
    }

    for (int i = 0; i < SYMBOL_TABLE_NODE_COUNT; i++)
    for (int j = 0; j < table.nodes[i].declaration_count; j++) {
        Declaration *decl = table.nodes[i].declarations[j];
        if (decl->type != DECLARATION_VAR) continue;
        symbol_table_declaration_init(&table, decl);
    }
    
    symbol_table_free(&table);
}
