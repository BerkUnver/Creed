
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "prelude.h"

void print_indent(int count) {
    for (int i = 0; i < count; i++) print("    ");
}
Location location_expand(Location begin, Location end) {
    return (Location) {
        .line_start = begin.line_start,
        .char_start = begin.char_start,
        .line_end = end.line_end,
        .char_end = end.char_end
    };
}

void location_print(Location location) {
    printf("(%i, %i) -> (%i, %i)", location.line_start + 1, location.char_start + 1, location.line_end + 1, location.char_end + 1);
}

void literal_print(Literal *literal) {
    switch (literal->type) {
        
        case LITERAL_STRING:
            putchar('"');
            int idx = 0;
            char *string = string_cache_get(literal->data.l_string);
            while (string[idx] != '\0') {
                print_literal_char(string[idx]);
                idx++;
            }
            putchar('"');
            break;

        case LITERAL_INT8:
            printf("%di8", literal->data.l_int8);
            break;

        case LITERAL_INT16:
            printf("%hii16", literal->data.l_int16);
            break;
        
        case LITERAL_INT:
            printf("%di", literal->data.l_int);
            break;
        
        case LITERAL_INT64:
            printf("%lldi64", literal->data.l_int64);
            break;
        
        case LITERAL_UINT8:
            printf("%uu8", literal->data.l_uint8);
            break;

        case LITERAL_UINT16:
            printf("%huu16", literal->data.l_uint16);
            break;
        
        case LITERAL_UINT:
            printf("%uu", literal->data.l_uint);
            break;
        
        case LITERAL_UINT64:
            printf("%lluu64", literal->data.l_uint64);
            break;

        case LITERAL_FLOAT:
            // todo: make so this prints out only as many chars as the token is in length.
            printf("%ff", literal->data.l_float);
            break;
        
        case LITERAL_FLOAT64:
            printf("%lff64", literal->data.l_float64);
            break;

        case LITERAL_CHAR:
            putchar('\'');
            print_literal_char(literal->data.l_char);
            putchar('\'');
            break;

        default:
            printf("[Unrecognized token typen %i]", literal->type);
            break;
    }
}

void print(const char *str) {
    fputs(str, stdout);
}

void print_literal_char(char c) {
    switch (c) {
        case '\\': print("\\\\"); break;
        case '\n': print("\\n"); break;
        case '\t': print("\\t"); break;
        case '\0': print("\\0"); break;
        case '\'': print("\\'"); break;
        case '\"': print("\\\""); break;
        case '\r': print("\\r"); break;
        default: putchar(c); break;
    }
}

void error_exit(Location location, const char *error) {
    location_print(location);
    print(": ");
    puts(error);
    exit(0);
}
