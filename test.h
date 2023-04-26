#ifndef MY_HEADER_H
#define MY_HEADER_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include "parser.h"

void status(const char * const file_name, struct stat * file_status);
void compile_source();
int main(int argc, char **argv);

#endif /* MY_HEADER_H */