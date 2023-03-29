#include <stdlib.h>
#include <string.h>
#include "string_cache.h"

// This is a very naive implementation for now.

#define STRINGS_LENGTH_DEFAULT 128
#define STRINGS_REALLOC_MULTIPLIER 1.5f
#define STRING_TABLE_LENGTH 1024 // TODO: Please change this so the table automatically resizes and balances itself instead of being a predefined size.

static unsigned long string_hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + (unsigned char) c;
    return hash;
}

static struct {
    StringNode *nodes;
    int node_count;
} string_table[STRING_TABLE_LENGTH]; // each node's node_count is automatically initialized to 0 so this is fine.

static char **strings;
static int strings_length = 0;
static int strings_length_alloc = STRINGS_LENGTH_DEFAULT;

void string_cache_init(void) {
    strings = malloc(sizeof(char *) * STRINGS_LENGTH_DEFAULT);
    // default lengths are initialized above.
}

void string_cache_free(void) {
    for (int i = 0; i < STRING_TABLE_LENGTH; i++) {
        for (int j = 0; j < string_table[i].node_count; j++) {
            free(string_table[i].nodes[j].string);
        }
        free(string_table[i].nodes);
    }
    free(strings);
}

// string is cloned into the hash table.
StringId string_cache_insert(const char *string) {

    unsigned long hash = string_hash_djb2(string);
    int idx = (int) (hash % (unsigned long) STRING_TABLE_LENGTH);
    
    for (int i = 0; i < string_table[idx].node_count; i++) {
        if (string_table[idx].nodes[i].hash == hash && !strcmp(string_table[idx].nodes[i].string, string)) {
            return string_table[idx].nodes[i].id;
        }
    }

    // insert the string into the StringId -> char* array;
    StringId id = { .idx = strings_length};
    strings_length++;
    if (strings_length > strings_length_alloc) {
        strings_length_alloc = (int) (strings_length_alloc * STRINGS_REALLOC_MULTIPLIER);
        strings = realloc(strings, strings_length_alloc * sizeof(char *));
    }

    char *string_copy = strcpy(malloc(sizeof(char) * (strlen(string) + 1)), string);  
    strings[id.idx] = string_copy;
    
    // insert the string into the hashtable
    string_table[idx].node_count++;
    string_table[idx].nodes = realloc(string_table[idx].nodes, sizeof(StringNode) * string_table[idx].node_count);
    string_table[idx].nodes[string_table[idx].node_count - 1] = (StringNode) {
        .string = string_copy,
        .hash = hash,
        .id = id
    };

    return id;
}

char *string_cache_get(StringId id) {
    return strings[id.idx];
}
