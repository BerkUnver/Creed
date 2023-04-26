#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

void status(const char * const file_name, struct stat * file_status)
{
    int ret = stat(file_name, file_status);
    if (ret != 0 ) {
        perror("Could not obtain file information");
        exit(EXIT_FAILURE);
    }
}

void compile_source() {
    //This func will compile using creed and then output a result
    //the result will then be used in the main function where assert is used 
    //with flag
}

int main(int argc, char **argv) {
    bool run_tests = false;

    // Check for -test flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-test") == 0) {
            run_tests = true;
            break;
        }
    }

    if (run_tests) {
        // Compile test files
        compile_source();

        // Check generated IR code against expected output
        assert(strcmp(generated_ir_code, expected_ir_code) == 0);
    } else {
        // Compile regular source code files
        compile_source();
    }

    return 0;
}
