#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "handlers.h"
#include "parser.h"
#include "string_cache.h"
#include "symbol_table.h"

int indent;
int array_count;

// Prints the appropriate number of 4-space indents
void write_indent(int count, FILE * outfile) {
    for (int i = 0; i < count; i++) {
        fprintf(outfile, "    ");     
    }
}

// TO DO: Figure out how to parse identifier for arrays
const char * get_complex_type(Type creadz_type) {
    const char * c_prim_type = get_type(*creadz_type.data.sub_type);

    if (creadz_type.type == TYPE_PTR || creadz_type.type == TYPE_PTR_NULLABLE) {
        char c_type[16];
        sprintf(c_type, "%s *", c_prim_type);
        const char * c_type_out = c_type;
        return c_type_out;
    }
    else if (creadz_type.type == TYPE_ARRAY) {

        return c_prim_type;
    }
    else {
        return c_prim_type;
    }
}

const char * get_type(Type creadz_type) {
    if (creadz_type.type == TYPE_PRIMITIVE) {
        TokenType creadz_prim_type = creadz_type.data.primitive;
        char * c_prim_type;

        switch(creadz_prim_type) {
            case TOKEN_KEYWORD_TYPE_VOID:
            case TOKEN_KEYWORD_TYPE_CHAR:
            case TOKEN_KEYWORD_TYPE_INT:
            case TOKEN_KEYWORD_TYPE_FLOAT:
                c_prim_type = (string_keywords[creadz_prim_type - TOKEN_KEYWORD_MIN]);
                break;

            case TOKEN_KEYWORD_TYPE_INT8:
                c_prim_type = "char";
                break;
            case TOKEN_KEYWORD_TYPE_INT16:
                c_prim_type = "short";
                break;
            case TOKEN_KEYWORD_TYPE_INT64:
                c_prim_type = "long long";
                break;
            case TOKEN_KEYWORD_TYPE_UINT8:
                c_prim_type = "unsigned char";
                break;
            case TOKEN_KEYWORD_TYPE_UINT16:
                c_prim_type = "unsigned short";
                break;
            case TOKEN_KEYWORD_TYPE_UINT:
                c_prim_type = "unsigned int";
                break;
            case TOKEN_KEYWORD_TYPE_UINT64:
                c_prim_type = "unsigned long long";
                break;
            case TOKEN_KEYWORD_TYPE_FLOAT64:
                c_prim_type = "double";
                break;
            case TOKEN_KEYWORD_TYPE_BOOL:
                c_prim_type = "int";
                break;
            default:
                assert(false);
        }
        return c_prim_type;
    }
    else if (creadz_type.type == TYPE_ARRAY || creadz_type.type == TYPE_PTR || creadz_type.type ==  TYPE_PTR_NULLABLE) {
        return get_complex_type(creadz_type);
    }
    else {
        return "";
    }
}

void handle_arrays(Type * type) {

}

void handle_literals(Literal * literal, FILE * outfile) {
    switch (literal->type) {
        case LITERAL_STRING:
            fputc('"', outfile);
            int idx = 0;
            char * string = string_cache_get(literal->data.l_string);
            while (string[idx] != '\0') {
                print_literal_char(string[idx]);
                idx++;
            }
            fputc('"', outfile);
            break;
        case LITERAL_INT8:
            fprintf(outfile, "%d", literal->data.l_int8);  
            break;     
        case LITERAL_INT16:
            fprintf(outfile, "%hi", literal->data.l_int16);     
            break;  
        case LITERAL_INT:
            fprintf(outfile, "%d", literal->data.l_int);     
            break;  
        case LITERAL_INT64:
            fprintf(outfile, "%lldll", literal->data.l_int64);       
            break; 
        case LITERAL_UINT8:
            fprintf(outfile, "%uu", literal->data.l_uint8);
            break;
        case LITERAL_UINT16:
            fprintf(outfile, "%huu", literal->data.l_uint16);
            break;
        case LITERAL_UINT:
            fprintf(outfile, "%uu", literal->data.l_uint);
            break;
        case LITERAL_UINT64:
            fprintf(outfile, "%lluull", literal->data.l_uint64);
            break;
        case LITERAL_FLOAT:
            fprintf(outfile, "%ff", literal->data.l_float);
            break;
        case LITERAL_FLOAT64:
            fprintf(outfile, "%lf", literal->data.l_float64);
            break;
        case LITERAL_CHAR:
            fprintf(outfile, "%c", literal->data.l_char);
            break;
    }
}

void handle_statement(Statement * statement, FILE * outfile) {
    switch (statement->type) {
        case STATEMENT_DECLARATION:
            handle_declaration(&statement->data.declaration, outfile);
            break;

        case STATEMENT_INCREMENT:
            fprintf(outfile, "++");
            handle_expr(&statement->data.increment, outfile);
            break;

        case STATEMENT_DEINCREMENT:
            fprintf(outfile, "--");
            handle_expr(&statement->data.deincrement, outfile);
            break;

        case STATEMENT_ASSIGN:
            handle_expr(&statement->data.assign.assignee, outfile);
            fprintf(outfile, " %s ", string_assigns[statement->data.assign.type - TOKEN_ASSIGN_MIN]);
            handle_expr(&statement->data.assign.value, outfile);
            break;

        case STATEMENT_EXPR:
            handle_expr(&statement->data.expr, outfile);
            break;

        case STATEMENT_LABEL:
            fprintf(outfile, "%s%c\n", string_cache_get(statement->data.label), TOKEN_COLON);
            break;

        case STATEMENT_LABEL_GOTO:
            fprintf(outfile, "goto %s%c", string_cache_get(statement->data.label_goto), TOKEN_SEMICOLON);
            break;

        case STATEMENT_RETURN:
            fprintf(outfile, "return");
            if (statement->data.return_value.exists) {
                fputc(' ', outfile);
                handle_expr(&statement->data.return_value.expr, outfile);
            }
    }
}

void handle_scope(Scope * scope, FILE * outfile) {
    if (scope->type != SCOPE_BLOCK) {
        write_indent(indent, outfile);
    }
    switch(scope->type) {
        case SCOPE_STATEMENT:
            handle_statement(&scope->data.statement, outfile);
            handle_statement_end(outfile);
            break;

        case SCOPE_CONDITIONAL:
            fprintf(outfile, "if (");
            handle_expr(&scope->data.conditional.condition, outfile);
            fprintf(outfile, ")");
            handle_scope(scope->data.conditional.scope_if, outfile);
            if (scope->data.conditional.scope_else) {
                fprintf(outfile, "\nelse ");
                handle_scope(scope->data.conditional.scope_else, outfile);
            }
            break;
            
        case SCOPE_LOOP_FOR:
            fprintf(outfile, "for (");
            handle_statement(&scope->data.loop_for.init, outfile);
            fprintf(outfile, "%c ", TOKEN_SEMICOLON);
            handle_expr(&scope->data.loop_for.expr, outfile);
            fprintf(outfile, "%c ", TOKEN_SEMICOLON);
            handle_statement(&scope->data.loop_for.step, outfile);
            fprintf(outfile, ") ");
            handle_scope(scope->data.loop_for.scope, outfile);
            break;
            
        // TO DO: Get size of array for iteration
        case SCOPE_LOOP_FOR_EACH:
            break;
            
        case SCOPE_LOOP_WHILE:
            fprintf(outfile, "while (");
            handle_expr(&scope->data.loop_while.expr, outfile);
            fprintf(outfile, ")");
            handle_scope(scope->data.loop_while.scope, outfile);
            break;

        case SCOPE_BLOCK:
            fprintf(outfile, " {\n");
            indent++;
            for (int i = 0; i < scope->data.block.scope_count; i++) {
                handle_scope(&scope->data.block.scopes[i], outfile);
            }
            indent--;
            write_indent(indent, outfile);
            fprintf(outfile, "}\n\n");

        // TO DO: Handle match statements?
        case SCOPE_MATCH:
            break;
    }
}   

void handle_expr(Expr * expr, FILE * outfile) {
    switch(expr->type) {
        case EXPR_PAREN:
            fputc(TOKEN_PAREN_OPEN, outfile);
            handle_expr(expr->data.parenthesized, outfile);
            fputc(TOKEN_PAREN_CLOSE, outfile);
            break;
        
        case EXPR_UNARY:
            fputc(expr->data.unary.type, outfile);
            handle_expr(expr->data.unary.operand, outfile);
            break;

        case EXPR_BINARY:
            handle_expr(expr->data.binary.lhs, outfile);
            fprintf(outfile, " %s ", string_operators[expr->data.binary.operator - TOKEN_OP_MIN]);
            handle_expr(expr->data.binary.rhs, outfile);
            break;

        case EXPR_TYPECAST:
            fputc(TOKEN_PAREN_OPEN, outfile);
            const char * type = get_type(expr->data.typecast.cast_to);
            fprintf(outfile, "%s", type);
            fputc(TOKEN_PAREN_CLOSE, outfile);
            handle_expr(expr->data.typecast.operand, outfile);
            break;

        case EXPR_ACCESS_MEMBER:
            handle_expr(expr->data.access_member.operand, outfile);
            fputc(TOKEN_DOT, outfile);
            fprintf(outfile, "%s", string_cache_get(expr->data.access_member.member));
            break;

        case EXPR_ACCESS_ARRAY:
            handle_expr(expr->data.access_array.operand, outfile);
            fputc(TOKEN_BRACKET_OPEN, outfile);
            handle_expr(expr->data.access_array.index, outfile);
            fputc(TOKEN_BRACKET_CLOSE, outfile);
            break;

        case EXPR_FUNCTION:
            fputc(TOKEN_PAREN_OPEN, outfile);
            if (expr->data.function.type.data.function.param_count > 0) {
                for (int i = 0; i < expr->data.function.type.data.function.param_count - 1; i++) {
                    const char * param_type = get_type(expr->data.function.type.data.function.params[i].type);
                    fprintf(outfile, "%s ", param_type);
                    fprintf(outfile, "%s", string_cache_get(expr->data.function.type.data.function.params[i].id));
                    fprintf(outfile, "%c ", TOKEN_COMMA);
                }
                const char * param_type = get_type(expr->data.function.type.data.function.params[expr->data.function.type.data.function.param_count - 1].type);
                fprintf(outfile, "%s ", param_type);
                fprintf(outfile, "%s", string_cache_get(expr->data.function.type.data.function.params[expr->data.function.type.data.function.param_count - 1].id));
            }
            fputc(TOKEN_PAREN_CLOSE, outfile);
            handle_scope(expr->data.function.scope, outfile);
            
            break;

        case EXPR_FUNCTION_CALL:
            handle_expr(expr->data.function_call.function, outfile);
            fputc(TOKEN_PAREN_OPEN, outfile);
            if (expr->data.function_call.param_count > 0) {
                int last_param_idx = expr->data.function_call.param_count - 1;
                for (int i = 0; i < last_param_idx; i++) {
                    handle_expr(&expr->data.function_call.params[i], outfile);
                    fprintf(outfile, "%c ", TOKEN_COMMA);
                }
                handle_expr(&expr->data.function_call.params[last_param_idx], outfile);
            }
            fputc(TOKEN_PAREN_CLOSE, outfile);
            break;

        case EXPR_ID:
            fprintf(outfile, "%s", string_cache_get(expr->data.id));
            break;

        case EXPR_LITERAL:
            handle_literals(&expr->data.literal, outfile);
            break;

        case EXPR_LITERAL_BOOL:
            if (expr->data.literal_bool == false) {
                fprintf(outfile, "%d", 0);
            }
            else if (expr->data.literal_bool == true) {
                fprintf(outfile, "%d", 1);
            }
            break;

        case EXPR_LITERAL_ARRAY:
            fputc(TOKEN_CURLY_BRACE_OPEN, outfile);
            if (expr->data.literal_array.allocated_count) {
                for (int i = 0; i < expr->data.literal_array.allocated_count-1; i++) {
                    handle_expr(&expr->data.literal_array.members[i], outfile);
                    fprintf(outfile, ", ");
                }
                handle_expr(&expr->data.literal_array.members[expr->data.literal_array.allocated_count-1], outfile);
            }

            fputc(TOKEN_CURLY_BRACE_CLOSE, outfile);
            break;
    }   
}

void handle_declaration(Declaration * declaration, FILE * outfile) {
    switch(declaration->type) {
        case DECLARATION_VAR:
            switch (declaration->data.var.type) {
                case DECLARATION_VAR_CONSTANT: {
                    if (!declaration->id.idx) {
                        error_exit(declaration->location, "Declaration must include an identifier.");
                    } 
                    if (declaration->data.var.data.constant.type_explicit) {
                        Type type = declaration->data.var.data.constant.type;
                        if (!type.type) {
                            error_exit(declaration->location, "Declaration must include a type.");
                        }
                        const char * type_str = get_type(type);
                        fprintf(outfile, "%s ", type_str);
                    }
                    else {
                        if (declaration->data.var.data.constant.value.type == EXPR_FUNCTION) {
                            Type type = *declaration->data.var.data.constant.value.data.function.type.data.function.result;
                            const char * type_str = get_type(type);
                            fprintf(outfile, "%s ", type_str);
                        }
                    }
                    const char * id = string_cache_get(declaration->id);
                    fprintf(outfile, "%s", id);
                    handle_expr(&declaration->data.var.data.constant.value, outfile);
                    break;
                }
                case DECLARATION_VAR_MUTABLE: {
                    const char * type = get_type(declaration->data.var.data.mutable.type);
                    const char * id = string_cache_get(declaration->id);
                    if (declaration->data.var.data.mutable.type.type == TYPE_ARRAY) {
                        if (declaration->data.var.data.mutable.value.data.literal_array.count != NULL) {
                            fprintf(outfile, "%s %s[", type, id);
                            handle_expr(declaration->data.var.data.mutable.value.data.literal_array.count, outfile);
                            fprintf(outfile, "]");
                        }
                        else {
                            fprintf(outfile, "%s %s[]", type, id);
                        }
                    }
                    else {
                        fprintf(outfile, "%s %s", type, id);
                    }
                    if (declaration->data.var.data.mutable.value_exists) {
                        fprintf(outfile, " %s ", string_assigns[TOKEN_ASSIGN - TOKEN_ASSIGN_MIN]);
                        handle_expr(&declaration->data.var.data.mutable.value, outfile);
                    }                   
                    break;
                }
            }
            break;

        // TO DO: Implement assigning a value to an enum?
        case DECLARATION_ENUM:
            fprintf(outfile, "enum %s %c\n", string_cache_get(declaration->id), TOKEN_CURLY_BRACE_OPEN);
            indent++;
            for (int i = 0; i < declaration->data.enumeration.member_count; i++) {
                write_indent(indent, outfile);
                fprintf(outfile, "%s%c\n", string_cache_get(declaration->data.enumeration.members[i]), TOKEN_COMMA);
            }
            indent--;
            write_indent(indent, outfile);
            fprintf(outfile, "%c", TOKEN_CURLY_BRACE_CLOSE);
            break;

        case DECLARATION_STRUCT:
            fprintf(outfile, "struct %s %c\n", string_cache_get(declaration->id), TOKEN_CURLY_BRACE_OPEN);
            indent++;
            for (int i = 0; i < declaration->data.struct_union.member_count; i++) {
                const char * member_type = get_type(declaration->data.struct_union.members[i].type);
                char * member_id = string_cache_get(declaration->data.struct_union.members[i].id);
                write_indent(indent, outfile);
                fprintf(outfile, "%s %s%c\n", member_type, member_id, TOKEN_SEMICOLON);
            }
            indent--;
            write_indent(indent, outfile);
            fprintf(outfile, "%c", TOKEN_CURLY_BRACE_CLOSE);
            break;

        case DECLARATION_UNION:
            fprintf(outfile, "union %s %c\n", string_cache_get(declaration->id), TOKEN_CURLY_BRACE_OPEN);
            indent++;
            for (int i = 0; i < declaration->data.struct_union.member_count; i++) {
                const char * member_type = get_type(declaration->data.struct_union.members[i].type);
                char * member_id = string_cache_get(declaration->data.struct_union.members[i].id);
                write_indent(indent, outfile);
                fprintf(outfile, "%s %s%c\n", member_type, member_id, TOKEN_SEMICOLON);
            }
            indent--;
            write_indent(indent, outfile);
            fprintf(outfile, "%c", TOKEN_CURLY_BRACE_CLOSE);
            break;

        // TO DO: Implement sum types?
        case DECLARATION_SUM:
            error_exit(declaration->location, "Translating this kind of declaration into C has not been implemented yet.");
            break;
    }
}

void handle_statement_end(FILE * outfile) {
    fprintf(outfile, ";\n");
}

void handle_driver(SourceFile * file) {
    remove("file.c");
    FILE * outfile = fopen("file.c", "w");
    if (outfile == NULL) {
        perror("Failed to open output file.");
        exit(EXIT_FAILURE);
    }

    indent = 0;
    // First pass
    for (int i = 0; i < file->declaration_count; i++) {
        if (file->declarations[i].type != DECLARATION_VAR) {
            handle_declaration(&file->declarations[i], outfile);
        }
    }
    // Second Pass
    for (int j = 0; j < file->declaration_count; j++) {
        if (file->declarations[j].type == DECLARATION_VAR) {
            handle_declaration(&file->declarations[j], outfile);
        }
    }
    fclose(outfile);
}   
