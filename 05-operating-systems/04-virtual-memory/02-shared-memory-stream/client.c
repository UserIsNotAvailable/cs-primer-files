#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "shm_blocking_queue.h"

#define SIZE (1 << 20) // Test with this many bytes of data
#define SHM_FNAME "mmap-stream"

int main() {
    int checksum, max = SIZE / sizeof(int), sock, n;
    char end_msg[4096];
    unsigned short port = 9876;
    struct timespec start, end;
    struct sockaddr_in server;

    // construct the client socket, and connect
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(port);

    shm_blocking_queue *sbq;
    shm_blocking_queue_init(SHM_FNAME, 4096);
    shm_blocking_queue_connect(&sbq, SHM_FNAME);

    connect(sock, (struct sockaddr *) &server, sizeof(server));
    send(sock, &max, 4, 0);
    send(sock, SHM_FNAME, strlen(SHM_FNAME), 0);

    // receive a bunch of data
    clock_gettime(CLOCK_MONOTONIC, &start);
    checksum = 0;
    for (int i = 0; i < max; i++) {
        n = (int) shm_blocking_queue_take(sbq);
        checksum ^= n;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    recv(sock, &end_msg, sizeof(end_msg), 0);
    close(sock);

    shm_blocking_queue_disconnect(sbq);
    shm_blocking_queue_destroy(SHM_FNAME);

    float secs =
            (float) (end.tv_nsec - start.tv_nsec) / 1e9 + (end.tv_sec - start.tv_sec);
    float mibs = (float) SIZE / secs / (1 << 20);
    printf("Received at %.3f MiB/s. Checksum: %d\n", mibs, checksum);
}
