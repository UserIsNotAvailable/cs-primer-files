#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_BYTES (1 << 20)

void error() {
    fprintf(stderr, strerror(errno));
    exit(EXIT_FAILURE);
}

int main(void) {
    const char *const fname = "./tmp";
    FILE *fp;

    if (NULL == (fp = fopen(fname, "w"))
        || 0 != setvbuf(fp, NULL, _IONBF, 0))
        error();

    off_t blks = 0;
    for (int i = 0; i != MAX_BYTES; ++i) {
        if (1 > fwrite("\0", sizeof(char), 1, fp)) error();

        struct stat st;
        if (0 != fstat(fp->_fileno, &st)) error();
        if (blks != st.st_blocks) {
            printf("size: %lld, blocks: %lld, disk usage: %lld\n",
                   st.st_size, st.st_blocks, st.st_blocks << 9);
            blks = st.st_blocks;
        }
    }

    if (0 != fclose(fp)
        || 0 != remove(fname))
        error();
    return 0;
}
