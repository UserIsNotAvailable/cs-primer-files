#include <signal.h>
#include <stdio.h>
#include <unistd.h>

volatile sig_atomic_t resized = 0;
void sigaction_handler(int unused) { resized = 1; }
void print_box() { printf("box\n"); }

int main(void) {
    struct sigaction act = {};
    sigaddset(&act.sa_mask, SIGWINCH);
    act.sa_handler = sigaction_handler;
    sigaction(SIGWINCH, &act, NULL);

    do {
        resized = 0;
        print_box();
        pause();
    } while (true);
}
