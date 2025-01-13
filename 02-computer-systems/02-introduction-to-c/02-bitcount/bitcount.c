#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <immintrin.h>

uint32_t bitcount(uint32_t n) {
    return __builtin_popcount(n);
    // uint32_t cnt = 0;
    // while (n) {
    //     // cnt += n & 0x01;
    //     // n >>= 1;
    //     n&=(n-1);
    //     ++cnt;
    // }
    // // printf("%d\n", cnt);
    // return cnt;
}

int main() {
    for (uint32_t i = 0; i < 10; ++i) {
        clock_t start_time = (float) clock() ;
        for (uint64_t j = 0; j < UINT64_MAX; ++j) {
            for (uint64_t k = 0; k < UINT64_MAX; ++k) {
                for (uint64_t l = 0; l < UINT64_MAX; ++l) {
                    for (uint64_t m = 0; m < UINT64_MAX; ++m) {
                        assert(bitcount(0) == 0);
                        assert(bitcount(1) == 1);
                        assert(bitcount(3) == 2);
                        assert(bitcount(8) == 1);
                        // harder case:
                        assert(bitcount(0xffffffff) == 32);
                    }
                }
            }
        }
        clock_t end_time = (float) clock() ;

        clock_t time_elapsed = end_time - start_time;
        printf("%2d :%ld\n", i, time_elapsed);
    }
    printf("OK\n");
}
