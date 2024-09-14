/**
 * hsh: Headache shell.
 * Complex control flow of long jump, pure procedure oriented programming style,
 * unexpected behaviors of unused APIs and my old friends: pointer, segfault and memleak
 * all together makes me headache.
 */
#include <errno.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define INIT_ARG_CNT 10
#define JMP_VAL_HANDLE_INPUT 1
#define JMP_VAL_RUN_CMD 1

void handle_err() {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
}

sigset_t set = {};
void block_sigint(void) { if (sigprocmask(SIG_BLOCK, &set, NULL)) handle_err(); }
void unblock_sigint(void) { if (sigprocmask(SIG_UNBLOCK, &set, NULL)) handle_err(); }

volatile pid_t pid = -1;
sigjmp_buf jmp_env_handle_input;
sigjmp_buf jmp_env_run_cmd;

void handler(const int sig) {
    if (0 >= pid) siglongjmp(jmp_env_handle_input, JMP_VAL_HANDLE_INPUT);
    kill(pid, sig);
    siglongjmp(jmp_env_run_cmd, JMP_VAL_RUN_CMD);
}

void setup_handler(void (*handler)(int)) { if (SIG_ERR == signal(SIGINT, handler)) handle_err(); }

char *handle_input(const siginfo_t *const child_info) {
    volatile char *in = NULL;
    const char *prompt = "\U0001F921";

    if (JMP_VAL_HANDLE_INPUT == sigsetjmp(jmp_env_handle_input, 1)) prompt = "\n\U0001F649";
    else {
        if (CLD_KILLED == child_info->si_code) prompt = "\n\U0001F52A";
        else if (0 != child_info->si_status) prompt = "\U0001F4A9";
    }

    size_t n = 0;
    do {
        free((void *) in);
        printf("%s", prompt);
        in = NULL;
        unblock_sigint();
        const int eof_flg = getline((char **) &in, &n, stdin);
        block_sigint();
        prompt = "\U0001F921";

        if (-1 == eof_flg) {
            printf("%s\n", "\n\U0001F480");
            free((void *) in);
            return NULL;
        }

        if (0 != strcmp((void *) in, "\n")) return (char *) in;
    } while (true);
}

char **parse_cmd(char *const line) {
    char **cmd;
    if (NULL == (cmd = calloc(sizeof(char *),INIT_ARG_CNT))) handle_err();

    static const char *delim = " \t\n";
    int sz = INIT_ARG_CNT;
    int n = 0;
    for (char *token = strtok(line, delim);
         NULL != token;
         cmd[n++] = token, token = strtok(NULL, delim))
        if (n == sz && NULL == (cmd = reallocarray(cmd, sizeof(char *), sz <<= 1))) handle_err();
    cmd[n] = NULL;

    return cmd;
}

int handle_builtin(char **cmd) {
    if (0 == strcmp(*cmd, "help")) {
        printf("%s\n", "\U0001F92A");
        return 1;
    }

    if (0 == strcmp(*cmd, "quit")) {
        printf("%s\n", "\U0001F480");
        exit(EXIT_SUCCESS);
    }

    return 0;
}

void run_cmd(char **cmd, siginfo_t *const child_info) {
    pid = fork();
    switch (pid) {
        case -1:
            handle_err();

        case 0:
            setup_handler(SIG_DFL);
            unblock_sigint();
            if (-1 == execvp(cmd[0], cmd)) handle_err();

        default:
            if (JMP_VAL_RUN_CMD != sigsetjmp(jmp_env_run_cmd, 1)) unblock_sigint();
            waitid(P_PID, pid, child_info, WEXITED);
            block_sigint();
            pid = -1;
    }
}

int main(void) {
    sigaddset(&set, SIGINT);
    block_sigint();
    setup_handler(handler);

    char *input = NULL;
    char **cmd = NULL;
    siginfo_t child_info = {};
    while (NULL != (input = handle_input(&child_info))) {
        child_info.si_code = CLD_EXITED;
        child_info.si_status = 0;

        cmd = parse_cmd(input);
        if (!handle_builtin(cmd)) run_cmd(cmd, &child_info);

        free(cmd);
        free(input);
    }
    return 0;
}
