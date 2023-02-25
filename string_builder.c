#include <math.h>
#include <stdlib.h>
#include "string_builder.h"


StringBuilder string_builder_new(void) {
    char *str = malloc(sizeof(char) * STRING_BUILDER_INITIAL_SIZE);
    str[0] = '\0';
    return (StringBuilder) {
        .alloc_length = STRING_BUILDER_INITIAL_SIZE,
        .length = 0,
        .raw = str
    };
}

void string_builder_add_char(StringBuilder *builder, char c) {
    int required_size = builder->length + 2;
    if (required_size > builder->alloc_length) {
        builder->alloc_length = (int) ceilf(required_size * STRING_BUILDER_SIZE_INCREASE);
        builder->raw = realloc(builder->raw, builder->alloc_length * sizeof(char));
    }
    builder->raw[builder->length] = c;
    builder->length++;
    builder->raw[builder->length] = '\0';
}

// You trade access to the builder in order to get access to the raw.
char *string_builder_free(StringBuilder *builder) {
    return builder->raw;
}
