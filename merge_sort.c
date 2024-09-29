#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void merge(int *src, int *dst, size_t s, size_t m, size_t e) {
    size_t i = s, s2 = m;
    while (s < m && s2 < e)
        dst[i++] = src[s] < src[s2] ? src[s++] : src[s2++];
    if (s < m)
        memcpy(dst + i, src + s, sizeof(int) * (m - s));
    else if (s2 < e)
        memcpy(dst + i, src + s2, sizeof(int) * (e - s2));
}

void sort(int *arr, size_t len) {
    int *wk = calloc(sizeof(int), len);
    assert(NULL != wk);

    size_t sz = 2;
    for (; sz < len << 1; sz <<= 1) {
        for (size_t s = 0; s < len; s += sz) {
            size_t m = s + (sz >> 1), e = s + sz;
            merge(arr, wk, s, m < len ? m : len, e < len ? e : len);
        }
        int *tmp = arr; arr = wk; wk = tmp;
    }

    if (0x5555555555555555 & sz) {
        int *tmp = arr; arr = wk; wk = tmp;
        memcpy(arr, wk, sizeof(int) * len);
    }
    free(wk);
}

#define LEN 0xfffffff
int main(void) {
    int *arr = calloc(sizeof(int), LEN);
    assert(NULL != arr);

    srand(time(NULL));
    for (int i = 0; i < LEN; ++i)
        arr[i] = rand();

    sort(arr, LEN);
    for (int i = 1; i < LEN; ++i)
        assert(arr[i - 1]  <= arr[i]);

    free(arr);
    return 0;
}
