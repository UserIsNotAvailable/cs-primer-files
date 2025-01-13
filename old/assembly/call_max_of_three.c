#include <stdio.h>
#include <inttypes.h>

extern int64_t max_of_three(int64_t, int64_t, int64_t);

int main(void) {
    int a = 0x80000001;
    int b=a-0x80000001;

    printf("%ld\n", max_of_three(1, 2, 3));
    printf("%ld\n", max_of_three(1, 2, -3));
    printf("%ld\n", max_of_three(1, -2, -3));
    printf("%ld\n", max_of_three(-1, -2, -3));
}
