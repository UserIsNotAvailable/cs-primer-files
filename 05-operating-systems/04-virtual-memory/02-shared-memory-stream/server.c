#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "shm_blocking_queue.h"

int main() {
    int checksum, max, sock, conn, n;
    char shm_fname[4096];
    unsigned short port = 9876;
    struct sockaddr_in self, client;
    unsigned int clientlen;

    // construct the server socket, bind and listen for connections
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&self, 0, sizeof(self));
    self.sin_family = AF_INET;
    self.sin_addr.s_addr = htonl(INADDR_ANY);
    self.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &self, sizeof(self));
    listen(sock, 10);

    srand(0x1234);
    // accept a new connection and stream it a bunch of random integers
    for (;;) {
        conn = accept(sock, (struct sockaddr *) &client, &clientlen);
        printf("Connection accepted\n");
        recv(conn, &max, 4, 0);
        recv(conn, &shm_fname, sizeof(shm_fname), 0);

        shm_blocking_queue *sbq;
        shm_blocking_queue_connect(&sbq, shm_fname);
        checksum = 0;
        for (int i = 0; i < max; i++) {
            n = rand();
            checksum ^= n;
            shm_blocking_queue_put(sbq, (void *) n);
        }
        shm_blocking_queue_disconnect(sbq);

        send(conn, "end", strlen("end"), 0);
        close(conn);
        printf("Sent %d random ints to client, checksum %d\n", max, checksum);
    }
}
