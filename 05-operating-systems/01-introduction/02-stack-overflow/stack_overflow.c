#include <stdio.h>

void main(int argc, char **argv) {
    printf("depth: %d, ptr: %p, size: %ld\n",
           argc, &argc, (char *) argv - (char *) &argc);
    main(++argc, argc > 2 ? argv : &argc);
}
