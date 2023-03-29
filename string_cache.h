#ifndef STRING_CACHE_H
#define STRING_CACHE_H

typedef struct StringId {
    int idx;
} StringId;

typedef struct StringNode {
    char *string;
    unsigned long hash;
    StringId id;
} StringNode;

void string_cache_init(void);
void string_cache_free(void);
StringId string_cache_insert(const char *string);
char *string_cache_get(StringId id);

#endif
