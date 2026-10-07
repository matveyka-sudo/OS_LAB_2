#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int result = 0;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        for (ssize_t i = 0; i < length; i++) {
            line[i] = (char)toupper((unsigned char)line[i]);
        }

        if (fwrite(line, 1, (size_t)length, stdout) != (size_t)length || fflush(stdout) == EOF) {
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
