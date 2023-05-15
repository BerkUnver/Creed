#include <stdio.h>

#include "parser.h"

void write_indent(int count, FILE * outfile);
const char * get_type(Type creadz_type);
void handle_scope(Scope * scope, FILE * outfile);
void handle_expr(Expr * expr, FILE * outfile);
void handle_declaration(Declaration * declaration, FILE * outfile);
void handle_statement_end(FILE * outfile);
void handle_driver(SourceFile * file);

