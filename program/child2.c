#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int result = 0;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        size_t output_length = 0;

        for (ssize_t i = 0; i < length; i++) {
            if (line[i] == ' ' && output_length > 0 && line[output_length - 1] == ' ') {
                continue;
            }

            line[output_length++] = line[i];
        }

        if (fwrite(line, 1, output_length, stdout) != output_length || fflush(stdout) == EOF) {
            perror("stdout");
            result = 1;
            break;
        }
    }

    if (!result && !feof(stdin)) {
        perror("stdin");
        result = 1;
    }

    free(line);
    return result;
}
