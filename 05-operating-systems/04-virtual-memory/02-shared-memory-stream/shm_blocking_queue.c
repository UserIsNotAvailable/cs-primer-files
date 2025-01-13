#include "shm_blocking_queue.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void shm_blocking_queue_init(const char *shm_fname, const size_t capacity) {
    int fd;
    shm_blocking_queue *tmp;
    exit_err_on(0 > (fd = shm_open(shm_fname, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR)), "init 0");
    const size_t len = sizeof(*tmp) + sizeof(void *) * capacity;
    exit_err_on(0 != ftruncate(fd, len), "init 1");
    exit_err_on(MAP_FAILED == (tmp = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)), "init 2");
    exit_err_on(0 != close(fd), "init 3");

    tmp->capacity = capacity;
    tmp->head = 0;
    tmp->tail = 0;

    pthread_mutexattr_t attr;
    exit_err_on(0 != pthread_mutexattr_init(&attr), "init 4");
    exit_err_on(0 != pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST), "init 5");
    exit_err_on(0 != pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED), "init 6");
    exit_err_on(0 != pthread_mutex_init(&tmp->mutex_head, &attr), "init 7");
    exit_err_on(0 != pthread_mutex_init(&tmp->mutex_tail, &attr), "init 8");

    exit_err_on(0 != sem_init(&tmp->sem_element, 1, 0), "init 9");
    exit_err_on(0 != sem_init(&tmp->sem_space, 1, capacity), "init 10");

    exit_err_on(0 != munmap(tmp, len), "init 11");
}

void shm_blocking_queue_destroy(const char *shm_fname) {
    int fd;
    exit_err_on(0 > (fd = shm_open(shm_fname, O_RDWR, S_IRUSR | S_IWUSR)), "destroy 0");
    struct stat sb;
    exit_err_on(0 != fstat(fd, &sb), "destroy 1");
    shm_blocking_queue *tmp;
    exit_err_on(MAP_FAILED == (tmp = mmap(NULL, sb.st_size,
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)), "destroy 2");
    exit_err_on(0 != close(fd), "destroy 3");

    exit_err_on(0 != sem_destroy(&tmp->sem_space), "destroy 4");
    exit_err_on(0 != sem_destroy(&tmp->sem_element), "destroy 5");

    exit_err_on(0 != pthread_mutex_destroy(&tmp->mutex_tail), "destroy 6");
    exit_err_on(0 != pthread_mutex_destroy(&tmp->mutex_head), "destroy 7");

    exit_err_on(0 != munmap(tmp, sizeof(*tmp) + sizeof(void *) * tmp->capacity), "destroy 8");
    exit_err_on(0 != shm_unlink(shm_fname), "destroy 9");
}

void shm_blocking_queue_connect(shm_blocking_queue **sbq, const char *shm_fname) {
    int fd;
    exit_err_on(0 > (fd = shm_open(shm_fname, O_RDWR, S_IRUSR | S_IWUSR)), "connect 0");
    struct stat sb;
    exit_err_on(0 != fstat(fd, &sb), "connect 1");
    exit_err_on(MAP_FAILED == (*sbq = mmap(NULL, sb.st_size,
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)), "connect 2");
    exit_err_on(0 != close(fd), "connect 3");
}

void shm_blocking_queue_disconnect(shm_blocking_queue *const sbq) {
    exit_err_on(0 != munmap(sbq, sizeof(*sbq) + sizeof(void *) * sbq->capacity), "disconnect 0");
}

void shm_blocking_queue_put(shm_blocking_queue *const sbq, const void *const e) {
    exit_err_on(0 != sem_wait(&sbq->sem_space), "put 0");
    if (EOWNERDEAD == pthread_mutex_lock(&sbq->mutex_tail))
        exit_err_on(0 != pthread_mutex_consistent(&sbq->mutex_tail), "put 1");

    ((const void **) (sbq + 1))[sbq->tail] = e;
    sbq->tail = (sbq->tail + 1) % sbq->capacity;

    exit_err_on(0 != pthread_mutex_unlock(&sbq->mutex_tail), "put 2");
    exit_err_on(0 != sem_post(&sbq->sem_element), "put 3");
}

void *shm_blocking_queue_take(shm_blocking_queue *const sbq) {
    exit_err_on(0 != sem_wait(&sbq->sem_element), "take 0");
    if (EOWNERDEAD == pthread_mutex_lock(&sbq->mutex_head))
        exit_err_on(0 != pthread_mutex_consistent(&sbq->mutex_head), "take 1");

    void *e = ((void **) (sbq + 1))[sbq->head];
    sbq->head = (sbq->head + 1) % sbq->capacity;

    exit_err_on(0 != pthread_mutex_unlock(&sbq->mutex_head), "take 2");
    exit_err_on(0 != sem_post(&sbq->sem_space), "take 3");
    return e;
}
