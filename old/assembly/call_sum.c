#include <stdio.h>

extern double sum(double [], long);

int main(void) {
    double test[] = {
        1.0, 2.1, 3.3, 4.5
    };
    printf("%f\n", sum(test, 0));
    printf("%f\n", sum(test, 1));
    printf("%f\n", sum(test, 2));
    printf("%f\n", sum(test, 3));
    printf("%f\n", sum(test, 4));
    return 0;
}
