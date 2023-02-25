#include <stdbool.h>
#include <stdio.h>
#include "print.h"

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
