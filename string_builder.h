#ifndef CREED_STRING_BUILDER_H
#define CREED_STRING_BUILDER_H

#define STRING_BUILDER_INITIAL_SIZE 11
#define STRING_BUILDER_SIZE_INCREASE 1.5f

typedef struct StringBuidler {
    int alloc_length; // private
    int length;
    char *raw; // private
} StringBuilder;

StringBuilder string_builder_new(void);
void string_builder_add_char(StringBuilder *builder, char c);
char *string_builder_free(StringBuilder *builder);

#endif
