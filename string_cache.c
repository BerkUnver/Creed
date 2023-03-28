#include <stdlib.h>
#include <string.h>
#include "string_cache.h"

// This is a very naive implementation for now.

#define STRINGS_LENGTH_DEFAULT 128
#define STRINGS_REALLOC_MULTIPLIER 1.5f
#define STRING_TABLE_LENGTH 1024 // TODO: Please change this so the table automatically resizes and balances itself instead of being a predefined size.

static unsigned long string_djb2_hash(char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
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

// this takes ownership of the string you pass in.
// DO NOT PASS IN STRING LITERALS! THIS WILL SEGFAULT WHEN IT TRIES TO FREE THEM!
StringId string_cache_insert(char *string) {
    unsigned long hash = string_djb2_hash(string);
    int idx = (int) (hash % (unsigned long) STRING_TABLE_LENGTH);
    
    for (int i = 0; i < string_table[idx].node_count; i++) {
        if (string_table[idx].nodes[i].hash == hash && !strcmp(string_table[idx].nodes[i].string, string)) {
            free(string);
            return string_table[idx].nodes[i].id;
        }
    }

    StringId id = { .id = strings_length};
    strings_length++;
    if (strings_length > strings_length_alloc) {
        strings_length_alloc = (int) (strings_length_alloc * STRINGS_REALLOC_MULTIPLIER);
        strings = realloc(strings, strings_length_alloc * sizeof(char *));
    }
    strings[id.id] = string;
    
    string_table[idx].node_count++;
    string_table[idx].nodes = realloc(string_table[idx].nodes, sizeof(StringNode) * string_table[idx].node_count);
    string_table[idx].nodes[string_table[idx].node_count - 1] = (StringNode) {
        .string = string,
        .hash = hash,
        .id = id
    };

    return id;
}

char *string_cache_get(StringId id) {
    return strings[id.id];
}
