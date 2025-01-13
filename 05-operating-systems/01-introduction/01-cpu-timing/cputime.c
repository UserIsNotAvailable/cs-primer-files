#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <sys/times.h>

#define SLEEP_SEC 3
#define NUM_MULS 100000000
#define NUM_MALLOCS 100000
#define MALLOC_SIZE 1000

typedef struct profile_info {
    char *title;
    clock_t rtime_start;
    clock_t utime_start;
    clock_t stime_start;
} profile_info;

char log_prefix[] = {"\x1b[3 m[pid %d, cpu %d] \x1b[0m"};

void profile_start(profile_info *t) {
    struct tms buf;
    t->rtime_start = times(&buf);
    t->utime_start = buf.tms_utime;
    t->stime_start = buf.tms_stime;

    log_prefix[3] = '0' + getpid() % 7 + 1;
    unsigned int cpu;
    getcpu(&cpu, nullptr);
    printf(log_prefix, getpid(), cpu);

    printf("%s\n", t->title);
}

void profile_log(const profile_info *t) {
    struct tms buf;
    const clock_t rtime_end = times(&buf);
    const clock_t utime_end = buf.tms_utime;
    const clock_t stime_end = buf.tms_stime;

    log_prefix[3] = '0' + getpid() % 7 + 1;
    unsigned int cpu;
    getcpu(&cpu, nullptr);
    printf(log_prefix, getpid(), cpu);

    const int tics_per_second = sysconf(_SC_CLK_TCK);
    printf("real:%.4f ,user: %.4f, sys:%.4f\n",
           (double) (rtime_end - t->rtime_start) / tics_per_second,
           (double) (utime_end - t->utime_start) / tics_per_second,
           (double) (stime_end - t->stime_start) / tics_per_second);
}

int main(void) {
    profile_info t;

    t.title = "profile doing a bunch of floating point muls";
    float x = 1.0;
    profile_start(&t);
    for (int i = 0; i < NUM_MULS; i++)
        x *= 1.1;
    profile_log(&t);

    t.title = "profile doing a bunch of mallocs";
    profile_start(&t);
    void *p;
    for (int i = 0; i < NUM_MALLOCS; i++)
        p = malloc(MALLOC_SIZE);
    profile_log(&t);

    t.title = "profile sleeping";
    profile_start(&t);
    sleep(SLEEP_SEC);
    profile_log(&t);

    return 0;
}
