typedef struct StringId {
    int id;
} StringId;

typedef struct StringNode {
    char *string;
    unsigned long hash;
    StringId id;
} StringNode;

void string_cache_init(void);
void string_cache_free(void);
StringId string_cache_insert(char *string);
char *string_cache_get(StringId id);
