/**
 * lsh: Lazy shell.
 * I'm so lazy to write one from scratch that I decided to stand on shoulders of Oz:
 * watch the whole video and twist Oz's solution.
 */
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROMPT 4096
#define MAX_ARGV 256
#define PROMPT "\xF0\x9F\x90\x9A "
#define PIPE "|"
#define SEP " \t\n"

void error(const int status) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(status);
}

// SIGINT forwarding for pipelines
volatile pid_t pgid = 0;
void sigint_handler(int sig) {
    if (!pgid) exit(0);
    if (killpg(pgid, SIGINT) < 0) error(1);
}

void run(char prompt[MAX_PROMPT], const int infd) {
    char *tok = strsep(&prompt, PIPE);

    int argc = 0;
    char *argv[MAX_ARGV];
    char *subtok = strtok(tok, SEP);
    while (true) {
        if (NULL == (argv[argc] = subtok)) break;
        subtok = strtok(NULL, SEP);
        ++argc;
    }
    if (0 == argc) return;

    int fds[2] = {fileno(stdin), fileno(stdout)};
    // do not create pipe for the last command
    if (NULL != prompt && 0 > pipe(fds)) error(2);

    const pid_t pid = fork();
    switch (pid) {
        case -1:
            error(3);

        case 0:
            // not the first command
            if (fileno(stdin) != infd) {
                if (0 > dup2(infd, fileno(stdin))) error(4);
                if (0 > close(infd)) error(5);
            }

            // not the last command
            if (fileno(stdout) != fds[1]) {
                if (0 > dup2(fds[1], fileno(stdout))) error(6);
                if (0 > close(fds[0])) error(7);
                if (0 > close(fds[1])) error(8);
            }

            if (0 > execvp(argv[0], argv)) error(9);
            error(10);

        default:
            pgid = (0 == pgid) ? pid : pgid;
            if (0 > setpgid(pid, pgid)) error(11);

            // not the last command
            if (fileno(stdout) != fds[1]) {
                if (0 > close(fds[1])) error(12);
                run(prompt, fds[0]);
                if (0 > close(fds[0])) error(13);
            }

            int status;
            waitpid(pid, &status, 0);
            pgid = (pid == pgid) ? 0 : pgid;
    }
}

int main() {
    char prompt[MAX_PROMPT];
    signal(SIGINT, sigint_handler);

    while (true) {
        printf(PROMPT);

        if ((NULL == fgets(prompt, MAX_PROMPT, stdin)) && ferror(stdin)) error(14);
        if (feof(stdin)) error(15);

        run(prompt, fileno(stdin));
    }
}
