#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sort(int *, size_t, size_t);

void merge(int *arr, size_t n, size_t border) {
    size_t left = 0, right = border, i;
    int *x = malloc(n * sizeof(int));
    // copy the ith item from either the left or right part
    for (i = 0; i < n; i++) {
        if (right == n)
            x[i] = arr[left++];
        else if (left == border)
            x[i] = arr[right++];
        else if (arr[right] < arr[left])
            x[i] = arr[right++];
        else
            x[i] = arr[left++];
    }
    // transfer from temporary array back to given one
    for (i = 0; i < n; i++)
        arr[i] = x[i];
    free(x);
}

typedef struct context {
    int *arr;
    int n;
    int cupno;
} context;

void *routine(void *arg) {
    context *ctx = arg;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_ISSET(ctx->cupno, &set);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set);
    sort(ctx->arr, ctx->n, 1);
    free(ctx);
    return NULL;
}

pthread_t mt_sort(int *arr, size_t n, size_t tcnt) {
    cpu_set_t set;
    CPU_ZERO(&set);
    pthread_getaffinity_np(pthread_self(), sizeof(set), &set);

    context *ctx = malloc(sizeof(context));
    ctx->arr = arr;
    ctx->n = n;
    ctx->cupno = 0;

    size_t mod = tcnt % CPU_COUNT(&set);
    for (; ctx->cupno < CPU_SETSIZE; ++ctx->cupno, --mod)
        if (CPU_ISSET(ctx->cupno, &set) && 0 == mod) break;

    pthread_t tid;
    pthread_create(&tid, NULL, routine, ctx);
    return tid;
}

void sort(int *arr, size_t n, size_t tcnt) {
    if (n < 2)
        return;

    size_t border = tcnt > 2 ? n / tcnt : n >> 1;

    pthread_t tid = 0;
    if (tcnt > 1)
        tid = mt_sort(arr, border, tcnt);
    else
        sort(arr, border, 1);
    sort(arr + border, n - border, tcnt > 2 ? tcnt - 1 : 1);

    if (tid)
        pthread_join(tid, NULL);
    merge(arr, n, border);
}

#define LEN 0xfffffff

int main(void) {
    int *arr = calloc(sizeof(int), LEN);
    assert(NULL != arr);

    srand(time(NULL));
    for (int i = 0; i < LEN; ++i)
        arr[i] = rand();

    sort(arr, LEN, 8);
    for (int i = 1; i < LEN; ++i)
        assert(arr[i - 1] <= arr[i]);

    free(arr);
    return 0;
}
