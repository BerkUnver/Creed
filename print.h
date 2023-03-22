#ifndef CREED_PRINT_H
#define CREED_PRINT_H

#include <stdbool.h>
#include "token.h"

void print(const char *str);
void print_literal_char(char c);
void error_exit(Location location, const char *error);

#endif
