#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <setjmp.h>

volatile uint64_t handled = 0;
sigjmp_buf jmp_env_seg;

void handle(int sig) {
    handled |= (1 << sig);
    printf("Caught %d: %s (%d total)\n", sig, strsignal(sig),
           __builtin_popcount(handled));

    static int i=1;
    switch (sig) {
        case SIGSEGV: siglongjmp(jmp_env_seg, i++);
        default:
            break;
    }
}


int main(int argc, char *argv[]) {
    // Register all valid signals
    for (int i = 0; i < NSIG; i++) {
        signal(i, handle);
    }

    if (0 == fork()) {
        exit(0);
    }

    int *p = 0;
    int ret;
    while (10> ( ret=sigsetjmp(jmp_env_seg, 1))) {
        printf("%d\n",ret);
        fflush(stdout);
        *p = 5;
    }

    // spin
    for (;;)
        sleep(1);
}
