#pragma once
#include <stdint.h>

struct profile_times {
    uint64_t wall_clock_usec;
    uint64_t user_cpu_usec;
    uint64_t kernel_cpu_usec;
};

void profile_start(struct profile_times *t);

void profile_log(struct profile_times *t);
