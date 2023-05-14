#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "prelude.h"

void print_indent(int count) {
    for (int i = 0; i < count; i++) print("    ");
}

Location location_expand(Location begin, Location end) {
    assert(begin.file_name.idx == end.file_name.idx);
    assert(begin.file_content.idx == end.file_content.idx);

    Location location = begin;
    location.idx_end = end.idx_end;
    return location;
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

#define CMD_RED "\x1B[31m"
#define CMD_GREEN "\x1B[32m"
#define CMD_RESET "\x1B[0m"

void error_exit(Location location, const char *error) {

    printf("Error! %s\n%s:%i\n", error, string_cache_get(location.file_name), location.idx_line + 1);
    
    const char *file = string_cache_get(location.file_content);
    int idx_start_line = location.idx_start;
    while (idx_start_line > 0 && file[idx_start_line - 1] != '\n') idx_start_line--;
    
    putchar('\n');
    print(CMD_GREEN);
    fwrite(file + idx_start_line, sizeof(char), location.idx_start - idx_start_line, stdout);
    
    print(CMD_RED);
    fwrite(file + location.idx_start, sizeof(char), location.idx_end - location.idx_start, stdout);
    print(CMD_GREEN);
    
    int idx_end_line = location.idx_end;
    while (file[idx_end_line] != '\n' && file[idx_end_line] != '\0') idx_end_line++;
    
    fwrite(file + location.idx_end, sizeof(char), idx_end_line - location.idx_end, stdout);
    print(CMD_RESET"\n\n");
    
    exit(EXIT_SUCCESS);
}
