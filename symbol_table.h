#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include <stdbool.h>
#include "string_cache.h"
#include "lexer.h"
#include "parser.h"

typedef struct SourceFileNode {
    Declaration *declarations;
    int declaration_count;
    int declaration_count_alloc;
} SourceFileNode;

#define SOURCE_FILE_NODE_COUNT 128
typedef struct SourceFile {
    StringId id;
    int import_count;
    StringId *imports;
    SourceFileNode nodes[SOURCE_FILE_NODE_COUNT];
} SourceFile;

#define SOURCE_FILE_FOR_EACH(decl, file) \
    for (int node_idx = 0; node_idx < SOURCE_FILE_NODE_COUNT; node_idx++) \
    for (decl = (file)->nodes[node_idx].declarations, int file_idx = 0; file_idx < (file)->nodes[node_idx].file_count; file_idx++, decl++)

typedef struct ProjectNode { 
    SourceFile *files;
    int file_count;
    int file_count_alloc;
} ProjectNode;

#define PROJECT_NODE_COUNT 512
typedef struct Project {
    ProjectNode nodes[PROJECT_NODE_COUNT];
} Project;

Project project_new(StringId path);
void project_free(Project *project);
SourceFile *project_get(Project *project, StringId path);
void project_print(Project *project, SourceFile *file);

typedef struct SymbolType {
    bool is_primitive;
    union {
        TokenType primitive;
        Declaration *declaration;
    } data;
} SymbolType;

bool symbol_type_equal(SymbolType lhs, SymbolType rhs);

typedef struct Symbol {
    StringId id;
    SymbolType type;
} Symbol;

typedef struct SymbolTableNode {
    int symbol_count;
    int symbol_count_alloc;
    Symbol *symbols;
} SymbolTableNode;

#define SYMBOL_TABLE_NODE_COUNT 32

typedef struct SymbolTable {
    SymbolTableNode nodes[SYMBOL_TABLE_NODE_COUNT];
    bool is_last;
    union {
        struct SymbolTable *previous;
        struct {
            SourceFile *file;
            Project *project;
        } last;
    } data;
} SymbolTable;

SymbolTable symbol_table_new(SymbolTable *previous); // previous can be null.
void symbol_table_free(SymbolTable *table); // frees only the first symbol table, not the previous ones.
bool symbol_table_get(SymbolTable *table, StringId id, Symbol *symbol);
bool symbol_table_add_var(SymbolTable *table, StringId id, Type *type);
void symbol_table_check_scope(SymbolTable *table, Scope *scope);

#endif
