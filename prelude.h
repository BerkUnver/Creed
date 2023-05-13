#ifndef CREED_PRELUDE_H
#define CREED_PRELUDE_H
// This defines commonly used constructs throughout the compiler.

#include "string_cache.h"

void print_indent(int count);

typedef struct Location {
    StringId file_name;
    StringId file_content;
    
    int idx_start;
    int idx_line;
    int idx_end;
} Location;

Location location_expand(Location begin, Location end);

typedef struct Literal {
    enum {
        LITERAL_STRING,
        LITERAL_CHAR,

        LITERAL_INT8,
        LITERAL_INT16,
        LITERAL_INT,
        LITERAL_INT64,
        
        LITERAL_UINT8,
        LITERAL_UINT16,
        LITERAL_UINT,
        LITERAL_UINT64,
        
        LITERAL_FLOAT,
        LITERAL_FLOAT64,
    } type;

    union {
        StringId l_string;
        char l_int8;
        short l_int16;
        int l_int;
        long long l_int64;
        unsigned char l_uint8;
        unsigned short l_uint16;
        unsigned int l_uint;
        unsigned long long l_uint64;
        float l_float;
        double l_float64;
        char l_char;
    } data;
} Literal; 

void literal_print(Literal *literal);

void print(const char *str);
void print_literal_char(char c);
void error_exit(Location location, const char *error);

#endif
