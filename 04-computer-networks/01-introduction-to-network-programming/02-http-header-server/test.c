#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define RT_SIGNAL SIGRTMIN // Real-time signal to use

int udp_socket; // Global socket file descriptor

void handle_rt_signal(int sig, siginfo_t *info, void *context) {
    (void)sig; // Suppress unused parameter warning
    (void)context;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    POLLIN
    // Receive data
    ssize_t bytes_received = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr *)&client_addr, &client_len);
    if (bytes_received < 0) {
        perror("recvfrom");
        return;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received message
    printf("Received message: %s\n", buffer);

    // Echo back the message
    ssize_t bytes_sent = sendto(udp_socket, buffer, bytes_received, 0,
                                (struct sockaddr *)&client_addr, client_len);
    if (bytes_sent < 0) {
        perror("sendto");
    } else {
        printf("Echoed back message: %s\n", buffer);
    }
}

int main() {
    struct sockaddr_in server_addr;

    // Create UDP socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set non-blocking mode
    int flags = fcntl(udp_socket, F_GETFL, 0);
    if (fcntl(udp_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl - non-blocking");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Set ownership of the socket to the process
    if (fcntl(udp_socket, F_SETOWN, getpid()) < 0) {
        perror("fcntl - setown");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Enable real-time signal for the socket
    if (fcntl(udp_socket, F_SETSIG, RT_SIGNAL) < 0) {
        perror("fcntl - setsig");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Allow real-time signals to be generated
    if (fcntl(udp_socket, F_SETFL, flags | O_ASYNC) < 0) {
        perror("fcntl - async");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a local address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Register real-time signal handler
    struct sigaction sa;
    sa.sa_sigaction = handle_rt_signal;
    sa.sa_flags = SA_SIGINFO; // Use the sa_sigaction field
    sigemptyset(&sa.sa_mask);
    if (sigaction(RT_SIGNAL, &sa, NULL) < 0) {
        perror("sigaction");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    printf("UDP echo server using real-time signals is running on port %d\n", PORT);

    // Keep the program running
    while (1) {
        pause(); // Wait for signals
    }

    close(udp_socket);
    return 0;
}
