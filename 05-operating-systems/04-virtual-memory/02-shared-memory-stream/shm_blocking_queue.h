#pragma once
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#define exit_err_on(expression, message) \
        do { \
            if ((expression)) { \
                fprintf(stderr, "%s:\n%s", message, strerror(errno)); \
                exit(EXIT_FAILURE); \
            } \
        } while (0)

typedef struct shm_blocking_queue {
    size_t capacity;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex_head;
    pthread_mutex_t mutex_tail;
    sem_t sem_element;
    sem_t sem_space;
} shm_blocking_queue;

void shm_blocking_queue_init(const char *shm_fname, size_t capacity);

void shm_blocking_queue_destroy(const char *shm_fname);

void shm_blocking_queue_connect(shm_blocking_queue **sbq, const char *shm_fname);

void shm_blocking_queue_disconnect(shm_blocking_queue *sbq);

void shm_blocking_queue_put(shm_blocking_queue *sbq, const void *e);

void *shm_blocking_queue_take(shm_blocking_queue *sbq);
