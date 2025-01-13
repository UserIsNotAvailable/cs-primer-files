#include <stdio.h>
#include <inttypes.h>
extern uint64_t factorial(uint64_t);

int main(void) {
    printf("%lu\n",factorial(1));
    printf("%lu\n",factorial(2));
    printf("%lu\n",factorial(3));
    printf("%lu\n",factorial(4));
    printf("%lu\n",factorial(5));
    printf("%lu\n",factorial(6));
    printf("%lu\n",factorial(7));
    printf("%lu\n",factorial(8));
    return 0;
}
