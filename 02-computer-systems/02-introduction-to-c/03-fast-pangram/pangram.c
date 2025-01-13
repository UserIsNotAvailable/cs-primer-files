#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define MASK 0x03fffffe
bool ispangram(const char *s) {
    uint32_t bits = 0;
    char ch;
    while ('\0' != (ch = *(s++))) {
        if (ch <= '@')continue;
        bits |= 1 << (ch & 0x1f);
        if (MASK == (bits & MASK))
            return true;
    }
    return false;
}

int main() {
    size_t len;
    ssize_t read;
    char *line = NULL;
    while ((read = getline(&line, &len, stdin)) != -1) {
        if (ispangram(line))
            printf("%s", line);
    }

    if (ferror(stdin))
        fprintf(stderr, "Error reading from stdin");

    free(line);
    fprintf(stderr, "ok\n");
}
