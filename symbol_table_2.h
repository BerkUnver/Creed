typedef struct TypeAnnotation {
    bool is_primitive;
    union {
        TokenType primitive;
        int type_info_idx;
    } data;
} TypeAnnotation;

typedef struct TypeInfo {
    enum {
        TYPE_INFO_ENUM,
        TYPE_INFO_STRUCT,
        TYPE_INFO_UNION,
        TYPE_INFO_SUM
    } type;
    
    enum {
        TYPE_INFO_STATE_UNINITIALIZED,
        TYPE_INFO_STATE_INITIALIZING,
        TYPE_INFO_STATE_INITIALIZED
    } state;

    union {
        struct {
            StringId *members;
            int member_count;
        } enumeration;

        struct {
            struct {
                StringId id;
                TypeAnnotation type;
            } *members;
            int member_count;
        } struct_union;

        struct {
            struct {
                StringId id;
                bool type_info_exists;
                TypeAnnotation type;
            } *members;
            int member_count;
        } sum;
    } data;
} TypeInfo;

typedef struct SymbolTableNode {
    Declaration *declarations;
    int declaration_count;
    int declaration_count_alloc;
} SymbolTableNode;

#define SYMBOL_TABLE_NODE_COUNT 64
typedef struct SymbolTable {
    TypeInfo *types;
    int type_count;
    SymbolTableNode nodes[SYMBOL_TABLE_NODE_COUNT];
} SymbolTable;

#define SYMBOL_TABLE_FOR_EACH(decl, table) \
    for (int node_idx = 0; node_idx < SYMBOL_TABLE_NODE_COUNT; node_idx++) \
    for (int decl_idx = (decl = (file)->nodes[node_idx].decl_count, 0); decl_idx < (file)->nodes[node_idx].decl_count; decl_idx++, decl++)
