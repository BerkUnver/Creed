#include <stdio.h>

#include "symbol_table.h"
#include "parser.h"

void handle_arithmetic_expr(Expr * expr);
void handle_semicolon();
void handle_function_declaration(Declaration * func);