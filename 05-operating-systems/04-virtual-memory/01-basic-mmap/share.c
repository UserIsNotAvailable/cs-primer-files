#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(void) {
    const size_t len = sizeof(int);
    int *addr = mmap(NULL, len, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == addr) {
        perror(strerror(errno));
        exit(EXIT_FAILURE);
    }
    *addr = 0;

    if (0 == fork()) {
        srand(time(NULL));
        // as the child, write a random number to shared memory
        *addr = rand();
        printf("Child has written %d to address %p\n", *addr, &addr);
        exit(EXIT_SUCCESS);
    }

    // as the parent, wait for the child and read out its number
    int stat;
    wait(&stat);
    printf("Parent reads %d from address %p\n", *addr, &addr);
    munmap(addr, len);
    return 0;
}
