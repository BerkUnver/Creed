#ifndef CREED_SYMBOL_TABLE_H
#define CREED_SYMBOL_TABLE_H

#include <stdbool.h>
#include "string_cache.h"
#include "lexer.h"
#include "parser.h"

typedef struct TypeListId {
    bool is_primitive;
    union {
        TokenType primitive;
        int idx;
    } data;
} TypeListId;

typedef struct TypeListMemberStructUnion {
    StringId id;
    TypeListId type;
} TypeListMemberStructUnion;

typedef struct TypeListMember {
    enum {
        SYMBOL_TYPE_STRUCT,
        SYMBOL_TYPE_UNION,
        SYMBOL_TYPE_ENUM,
        SYMBOL_TYPE_SUM
    } type;
    
    union {
        struct {
            TypeListMemberStructUnion *members;
            int member_count;
        } s_struct_union;
        
        struct {
            struct {
                StringId id;
                bool type_exists;
                TypeListId type;
            } *members;
            int member_count;
        } s_sum;

        struct {
            StringId *members;
            int member_count;
        } s_enum;
    } data;
} TypeListMember;

typedef struct TypeList {
    TypeListMember *types;
    int type_count;
    int type_count_alloc;
} TypeList;

typedef struct Symbol {
    StringId id;

    enum {
        SYMBOL_VAR,
        SYMBOL_DECLARATION
    } type;

    union {
        TokenType var_type;
        Declaration declaration;
    } data;
} Symbol;

typedef struct SymbolNode {
    int symbol_count;
    int symbol_count_alloc;
    Symbol *symbols;
} SymbolNode;

#define SYMBOL_TABLE_NODE_COUNT 128

typedef struct SymbolTable {
    SymbolNode nodes[SYMBOL_TABLE_NODE_COUNT];
    struct SymbolTable *previous;
} SymbolTable;

SymbolTable symbol_table_new(SymbolTable *previous); // previous can be null.
void symbol_table_free_head(SymbolTable *table); // frees only the first symbol table, not the previous ones.
bool symbol_table_has(const SymbolTable *table, StringId id);
bool symbol_table_get(const SymbolTable *table, StringId id, Symbol *symbol);
bool symbol_table_add_var(SymbolTable *table, StringId id, Type *type);
void symbol_table_check_scope(SymbolTable *table, Scope *scope);
void symbol_table_check_functions(SymbolTable *table);
void symbol_table_print(SymbolTable *table);
SymbolTable symbol_table_from_file(const char *path);
#endif
