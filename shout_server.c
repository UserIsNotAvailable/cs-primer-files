/**
 * Tested on Linux 5.14 and FreeBSD 14.2.
 */
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/epoll.h>
#elifdef __unix__
#include <sys/event.h>
#endif

#define PORT "8888"
#define MAX_LISTEN_SOCK_FDS 4
#define LISTEN_BACKLOG 10
#define MAX_EVENTS 100
#define BUF_CAPACITY 4096

typedef struct buffer {
    size_t size;
    char data[BUF_CAPACITY];
} buffer;

int init_listen_fds(const char* port, int lfds[], const int max_size) {
    struct addrinfo hints = {0}, *server_info;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int ecode;
    if (0 != (ecode = getaddrinfo(NULL, port, &hints, &server_info))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        return -1;
    }

    int nlfd = 0;
    for (const struct addrinfo* rp = server_info; NULL != rp; rp = rp->ai_next) {
        int lfd;
        if (0 > (lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))) {
            fprintf(stderr, "socket: %s\n", strerror(errno));
            continue;
        }

        if (0 > setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
            close(lfd);
            fprintf(stderr, "setsockopt 0: %s\n", strerror(errno));
            continue;
        }

        if (0 > setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int))) {
            close(lfd);
            fprintf(stderr, "setsockopt 1: %s\n", strerror(errno));
            continue;
        }

        if (0 > bind(lfd, rp->ai_addr, rp->ai_addrlen)) {
            close(lfd);
            fprintf(stderr, "bind: %s\n", strerror(errno));
            continue;
        }

        lfds[nlfd++] = lfd;
        if (max_size <= nlfd)
            break;
    }

    freeaddrinfo(server_info);
    return nlfd;
}

int set_nonblocking(const int fd) {
    int flags;
    if (0 > (flags = fcntl(fd, F_GETFL))) {
        fprintf(stderr, "fcntl 0: %s\n", strerror(errno));
        return -1;
    }

    if (0 > fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
        fprintf(stderr, "fcntl 1: %s\n", strerror(errno));
        return -2;
    }
    return 0;
}

bool is_lsockfd(const int fd, const int lsockfds[], const int nlsockfd) {
    for (int i = 0; i < nlsockfd; ++i)
        if (fd == lsockfds[i])
            return true;
    return false;
}

ssize_t do_send(const int fd, const char* data, size_t left) {
    ssize_t total_sent = 0;
    ssize_t sent = 0;
    while (0 < left) {
        sent = send(fd, data + sent, left, 0);
        if (0 >= sent) {
            if (EAGAIN == errno || EWOULDBLOCK == errno)
                return total_sent;

            fprintf(stderr, "send: %s\n", strerror(errno));
            return -1;
        }

        total_sent += sent;
        left -= sent;
    }

    return total_sent;
}

void to_upper_case(char* data, size_t size) {
    // TODO: Imagine I implemented this.
}

#ifdef __linux__
typedef struct user_data {
    int fd;
    buffer* buffer;
} user_data;

int ep_add(const int epfd, const int fd, const uint32_t evs, buffer* buf) {
    user_data* pud;
    if (NULL == (pud = calloc(1, sizeof(user_data)))) {
        fprintf(stderr, "calloc 0: %s\n", strerror(errno));
        return -1;
    }

    pud->fd = fd;
    pud->buffer = buf;

    struct epoll_event ee;
    ee.events = evs;
    ee.data.ptr = pud;
    if (0 > epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ee)) {
        fprintf(stderr, "epoll_ctl 0: %s\n", strerror(errno));
        free(pud);
        return -3;
    }

    return 0;
}

void cleanup(const int epfd, user_data* pud) {
    if (0 > epoll_ctl(epfd, EPOLL_CTL_DEL, pud->fd, NULL))
        fprintf(stderr, "epoll_ctl 1: %s\n", strerror(errno));

    close(pud->fd);

    if (pud->buffer)
        free(pud->buffer);
    free(pud);
}

int poll(int* lsockfds, int nlsockfd) {
    int epfd;
    if (0 > (epfd = epoll_create1(0))) {
        fprintf(stderr, "epoll_create1: %s\n", strerror(errno));
        return 4;
    }

    for (int i = 0; i < nlsockfd; ++i)
        if (0 > ep_add(epfd, lsockfds[i], EPOLLIN | EPOLLET, NULL))
            return 5;

    struct epoll_event evs[MAX_EVENTS] = {0};
    while (true) {
        int nfds;

        if (0 > (nfds = epoll_wait(epfd, evs, MAX_EVENTS, -1))) {
            fprintf(stderr, "epoll_wait: %s\n", strerror(errno));
            return 6;
        }

        for (int i = 0; i < nfds; ++i) {
            const uint32_t ev = evs[i].events;
            user_data* pud = evs[i].data.ptr;

            if (is_lsockfd(pud->fd, lsockfds, nlsockfd)) {
                while (true) {
                    struct sockaddr_storage cli_addr;
                    socklen_t len = sizeof(cli_addr);
                    int conn_sockfd;
                    if (0 > (conn_sockfd = accept(pud->fd, (struct sockaddr*)&cli_addr, &len))) {
                        if (EAGAIN != errno && EWOULDBLOCK != errno)
                            fprintf(stderr, "accept: %s\n", strerror(errno));
                        break;
                    }

                    if (0 > set_nonblocking(conn_sockfd)) {
                        close(conn_sockfd);
                        break;
                    }

                    buffer* buf;
                    if (NULL == (buf = calloc(1, sizeof(buffer)))) {
                        fprintf(stderr, "calloc 1: %s\n", strerror(errno));
                        close(conn_sockfd);
                        break;
                    }

                    if (0 > ep_add(epfd, conn_sockfd,
                                   EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP, buf)) {
                        free(buf);
                        close(conn_sockfd);
                        break;
                    }
                }
            } else if (ev & EPOLLRDHUP || ev & EPOLLHUP) {
                cleanup(epfd, pud);
            } else if (ev & EPOLLIN || ev & EPOLLOUT) {
                buffer* const buf = pud->buffer;
                if (ev & EPOLLOUT && 0 < buf->size) {
                    ssize_t sent = 0;
                    if (0 >= (sent = do_send(pud->fd, buf->data, buf->size))) {
                        cleanup(epfd, pud);
                        continue;
                    }

                    if (sent < buf->size) {
                        buf->size -= sent;
                        memmove(buf->data, buf->data + sent, buf->size);
                    } else
                        buf->size = 0;
                }

                if (ev & EPOLLIN && 0 == buf->size)
                    while (true) {
                        const ssize_t received = recv(pud->fd, buf->data, sizeof(buf->data), 0);

                        if (0 > received) {
                            if (EAGAIN != errno && EWOULDBLOCK != errno) {
                                fprintf(stderr, "recv: %s\n", strerror(errno));
                                cleanup(epfd, pud);
                            }
                            break;
                        }

                        if (0 == received) {
                            cleanup(epfd, pud);
                            break;
                        }

                        buf->size = received;
                        to_upper_case(buf->data, buf->size);

                        ssize_t sent = 0;
                        if (0 >= (sent = do_send(pud->fd, buf->data, buf->size))) {
                            cleanup(epfd, pud);
                            break;
                        }

                        if (sent < buf->size) {
                            buf->size -= sent;
                            memmove(buf->data, buf->data + sent, buf->size);
                            break;
                        }

                        buf->size = 0;
                    }
            } else {
                fprintf(stderr, "unknown error. fd: %d, events: %u\n", pud->fd, ev);
                cleanup(epfd, pud);
                goto end;
            }
        }
    }

end:
    close(epfd);
    return 0;
}
#elifdef __unix__
int kq_change(const int kq, const int fd, const short filter, const unsigned short flags, void* udata) {
    struct kevent ev;
    EV_SET(&ev, fd, filter, flags, 0, 0, udata);
    if (0 > kevent(kq, &ev, 1, NULL, 0, NULL)) {
        fprintf(stderr, "kevent 0: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void cleanup(const int kq, const int fd, buffer* buf) {
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(kq, &ev, 1, NULL, 0, NULL);

    EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    kevent(kq, &ev, 1, NULL, 0, NULL);

    close(fd);

    if (buf)
        free(buf);
}

int queue(int* lsockfds, int nlsockfd) {
    int kq;
    if (0 > (kq = kqueuex(0))) {
        fprintf(stderr, "kqueue: %s\n", strerror(errno));
        return 4;
    }

    for (int i = 0; i < nlsockfd; ++i)
        if (0 > kq_change(kq, lsockfds[i],EVFILT_READ, EV_ADD | EV_ENABLE, NULL))
            return 5;

    struct kevent evs[MAX_EVENTS] = {0};
    while (true) {
        int nev;

        if (0 > (nev = kevent(kq, NULL, 0, evs, MAX_EVENTS, NULL))) {
            fprintf(stderr, "kevent 1: %s\n", strerror(errno));
            return 6;
        }

        for (int i = 0; i < nev; ++i) {
            const int fd = (int)evs[i].ident;
            const short filter = evs[i].filter;
            const unsigned short flags = evs[i].flags;
            const unsigned int fflags = evs[i].fflags;
            buffer* buf = evs[i].udata;

            if (fflags & EV_EOF || fflags & EV_ERROR) {
                cleanup(kq, fd, buf);
            } else if (is_lsockfd(fd, lsockfds, nlsockfd)) {
                while (true) {
                    struct sockaddr_storage cli_addr;
                    socklen_t len = sizeof(cli_addr);
                    int conn_sockfd;
                    if (0 > (conn_sockfd = accept(fd, (struct sockaddr*)&cli_addr, &len))) {
                        if (EAGAIN != errno && EWOULDBLOCK != errno)
                            fprintf(stderr, "accept: %s\n", strerror(errno));

                        break;
                    }
                    if (0 > set_nonblocking(conn_sockfd)) {
                        close(conn_sockfd);
                        break;
                    }

                    buffer* new_buf;
                    if (NULL == (new_buf = calloc(1, sizeof(buffer)))) {
                        fprintf(stderr, "calloc: %s\n", strerror(errno));
                        break;
                    }

                    if (0 > kq_change(kq, conn_sockfd,EVFILT_READ, EV_ADD | EV_ENABLE, new_buf)) {
                        close(conn_sockfd);
                        free(new_buf);
                        break;
                    }
                }
            } else if (EVFILT_WRITE == filter && 0 < buf->size) {
                ssize_t sent = 0;
                if (0 >= (sent = do_send(fd, buf->data, buf->size))) {
                    cleanup(kq, fd, buf);
                    continue;
                }

                if (sent < buf->size) {
                    buf->size -= sent;
                    memmove(buf->data, buf->data + sent, buf->size);
                } else {
                    buf->size = 0;

                    if (0 > kq_change(kq, fd, EVFILT_WRITE, EV_DISABLE | EV_KEEPUDATA, NULL)) {
                        cleanup(kq, fd, buf);
                        continue;
                    }

                    if (0 > kq_change(kq, fd, EVFILT_READ, EV_ENABLE | EV_KEEPUDATA, NULL))
                        cleanup(kq, fd, buf);
                }
            } else if (EVFILT_READ == filter && 0 == buf->size) {
                while (true) {
                    const ssize_t received = recv(fd, buf->data, sizeof(buf->data), 0);

                    if (0 > received) {
                        if (EAGAIN != errno && EWOULDBLOCK != errno) {
                            fprintf(stderr, "recv: %s\n", strerror(errno));
                            cleanup(kq, fd, buf);
                        }

                        break;
                    }

                    if (0 == received) {
                        cleanup(kq, fd, buf);
                        break;
                    }

                    buf->size = received;
                    to_upper_case(buf->data, buf->size);

                    ssize_t sent = 0;
                    if (0 >= (sent = do_send(fd, buf->data, buf->size))) {
                        cleanup(kq, fd, buf);
                        break;
                    }

                    if (sent < buf->size) {
                        buf->size -= sent;
                        memmove(buf->data, buf->data + sent, buf->size);

                        if (0 > kq_change(kq, fd, EVFILT_READ, EV_DISABLE | EV_KEEPUDATA, NULL)) {
                            cleanup(kq, fd, buf);
                            break;
                        }

                        if (0 > kq_change(kq, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, buf))
                            cleanup(kq, fd, buf);

                        break;
                    }

                    buf->size = 0;
                }
            } else {
                fprintf(stderr, "unknown error. fd: %d, filter: %hd, flags: %hu, fflags: %u\n",
                        fd, filter, flags, fflags);
                cleanup(kq, fd, buf);
                goto end;
            }
        }
    }

end:
    close(kq);
    return 0;
}
#endif

int main(void) {
    int lsockfds[MAX_LISTEN_SOCK_FDS] = {0};
    int nlsockfd;

    if (0 >= (nlsockfd = init_listen_fds(PORT, lsockfds, MAX_LISTEN_SOCK_FDS)))
        return 1;

    for (int i = 0; i < nlsockfd; ++i)
        if (0 > set_nonblocking(lsockfds[i]))
            return 2;

    for (int i = 0; i < nlsockfd; ++i)
        if (0 > listen(lsockfds[i],LISTEN_BACKLOG)) {
            fprintf(stderr, "listen: %s\n", strerror(errno));
            return 3;
        }

    int ret = 0;
#ifdef __linux__
    ret = poll(lsockfds, nlsockfd);
#elifdef __unix__
    ret = queue(lsockfds, nlsockfd);
#endif

    for (int i = 0; i < nlsockfd; ++i)
        close(lsockfds[i]);

    return ret;
}
