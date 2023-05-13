#include <stdio.h>

#include "parser.h"

void handle_expr(Expr * expr);
void handle_arithmetic_expr(Expr * lhs, Expr * rhs, TokenType op);
void handle_statement_end();

