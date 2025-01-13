#include <cpuid.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include "cputime.h"

#define SLEEP_SEC 3
#define NUM_MULS 100000000
#define NUM_MALLOCS 1000000
#define MALLOC_SIZE 100000
#define TOTAL_USEC(tv) 1000000L * (tv).tv_sec + (tv).tv_usec

unsigned int getcpuid() {
    unsigned reg[4];
    __cpuid_count(1, 0, reg[0], reg[1], reg[2], reg[3]);
    return reg[1] >> 24;
}

// Populate the given struct with starting information
void profile_start(struct profile_times *t) {
    struct timeval tv;
    struct rusage ru;
    gettimeofday(&tv, NULL);
    getrusage(RUSAGE_SELF, &ru);
    t->wall_clock_usec = TOTAL_USEC(tv);
    t->user_cpu_usec = TOTAL_USEC(ru.ru_utime);
    t->kernel_cpu_usec = TOTAL_USEC(ru.ru_stime);
}

// Given starting information, compute and log differences to now
void profile_log(struct profile_times *t) {
    struct timeval tv;
    struct rusage ru;
    gettimeofday(&tv, NULL);
    getrusage(RUSAGE_SELF, &ru);

    uint64_t d_wall_clock = TOTAL_USEC(tv) - t->wall_clock_usec;
    uint64_t d_user_cpu = TOTAL_USEC(ru.ru_utime) - t->user_cpu_usec;
    uint64_t d_kernel_cpu = TOTAL_USEC(ru.ru_stime) - t->kernel_cpu_usec;

    fprintf(stderr, "[pid %d, cpu %d] real: %.03fs user: %0.03fs sys: %0.03f\n",
            getpid(), getcpuid(), d_wall_clock / 1000000.0,
            d_user_cpu / 1000000.0, d_kernel_cpu / 1000000.0);
}
