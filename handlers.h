#include <stdio.h>

#include "parser.h"

const char * get_type(Type creadz_type);
void handle_expr(Expr * expr);
void handle_arithmetic_expr(Expr * lhs, Expr * rhs, TokenType op);
void handle_declaration(Declaration * declaration);
void handle_statement_end();
void handle_driver(SourceFile * file);

