#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include <stdbool.h>
#include "string_cache.h"
#include "lexer.h"
#include "parser.h"

typedef struct SourceFileMember {
    StringId id;
    int idx;
} SourceFileMember;

typedef struct SourceFileNode {
    SourceFileMember *members;
    int member_count;
    int member_count_alloc;
} SourceFileNode;

#define SOURCE_FILE_NODE_COUNT 128
typedef struct SourceFile {
    StringId id;
    int import_count;
    StringId *imports;
    SourceFileNode nodes[SOURCE_FILE_NODE_COUNT];
} SourceFile;

void source_file_print(SourceFile *file);
void source_file_add(StringId path);
void source_file_table_clear();
SourceFile *source_file_table_get(StringId path);
void source_file_table_free();

typedef struct SymbolType {
    bool is_primitive;
    union {
        TokenType primitive;
        int declaration_idx;
    } data;
} SymbolType;

bool symbol_type_equal(SymbolType lhs, SymbolType rhs);


typedef struct Symbol {
    StringId id;
    SymbolType type;
} Symbol;


typedef struct SymbolNode {
    int symbol_count;
    int symbol_count_alloc;
    Symbol *symbols;
} SymbolNode;

#define SYMBOL_TABLE_NODE_COUNT 32

typedef struct SymbolTable {
    SymbolNode nodes[SYMBOL_TABLE_NODE_COUNT];
    struct SymbolTable *previous;
} SymbolTable;


SymbolTable symbol_table_new(SymbolTable *previous); // previous can be null.
void symbol_table_free_head(SymbolTable *table); // frees only the first symbol table, not the previous ones.
bool symbol_table_get(const SymbolTable *table, StringId id, Symbol *symbol);
bool symbol_table_add_var(SymbolTable *table, SourceFile *file, StringId id, Type *type);
void symbol_table_check_scope(SymbolTable *table, SourceFile *file, Scope *scope);

#endif
