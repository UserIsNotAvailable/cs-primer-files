#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct blocking_queue {
    const void **buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    pthread_mutex_t *mutex_head;
    pthread_mutex_t *mutex_tail;
    sem_t *sem_element;
    sem_t *sem_space;
} blocking_queue;

void blocking_queue_init(blocking_queue *const bq, const size_t capacity) {
    bq->buffer = malloc(capacity * sizeof(void *));
    bq->capacity = capacity;
    bq->head = 0;
    bq->tail = 0;
    bq->mutex_head = malloc(sizeof(pthread_mutex_t));
    bq->mutex_tail = malloc(sizeof(pthread_mutex_t));
    bq->sem_element = malloc(sizeof(sem_t));
    bq->sem_space = malloc(sizeof(sem_t));
    pthread_mutex_init(bq->mutex_head, NULL);
    pthread_mutex_init(bq->mutex_tail, NULL);
    sem_init(bq->sem_element, 0, 0);
    sem_init(bq->sem_space, 0, capacity);
}

void blocking_queue_destroy(const blocking_queue *const bq) {
    sem_destroy(bq->sem_space);
    sem_destroy(bq->sem_element);
    pthread_mutex_destroy(bq->mutex_tail);
    pthread_mutex_destroy(bq->mutex_head);
    free(bq->sem_space);
    free(bq->sem_element);
    free(bq->mutex_tail);
    free(bq->mutex_head);
    free(bq->buffer);
}

void blocking_queue_put(blocking_queue *const bq, const void *const e) {
    sem_wait(bq->sem_space);
    pthread_mutex_lock(bq->mutex_tail);
    bq->buffer[bq->tail] = e;
    printf("%d %p -> [%lu]\n", gettid(), e, bq->tail);
    bq->tail = (bq->tail + 1) % bq->capacity;
    pthread_mutex_unlock(bq->mutex_tail);
    sem_post(bq->sem_element);
}

const void *blocking_queue_take(blocking_queue *const bq) {
    sem_wait(bq->sem_element);
    pthread_mutex_lock(bq->mutex_head);
    const void *e = bq->buffer[bq->head];
    printf("%d     [%lu] -> %p\n", gettid(), bq->head, e);
    bq->head = (bq->head + 1) % bq->capacity;
    pthread_mutex_unlock(bq->mutex_head);
    sem_post(bq->sem_space);
    return e;
}

void *producer(void *arg) {
    blocking_queue *bq = arg;
    int i = 0;
    while (1)
        blocking_queue_put(bq, (void *) (i++ % 100));
}

void *consumer(void *arg) {
    blocking_queue *bq = arg;
    while (1)
        blocking_queue_take(bq);
}

int main() {
    printf("Starting basic test\n");
    blocking_queue *bq = malloc(sizeof(blocking_queue));
    blocking_queue_init(bq, 8);
    int i;
    for (i = 0; i < 5; i++)
        blocking_queue_put(bq, (void *) i);
    for (i = 0; i < 5; i++)
        blocking_queue_take(bq);
    for (i = 0; i < 5; i++)
        blocking_queue_put(bq, (void *) i);
    for (i = 0; i < 5; i++)
        blocking_queue_take(bq);
    blocking_queue_destroy(bq);
    printf("Starting concurrent test\n");
    // start n producers, m consumers in threads
    // producer just write incrementing integers to bq indefinitely
    // consumers just read/print them
    int n = 2;
    int m = 2;
    pthread_t prod[n], cons[m];
    blocking_queue_init(bq, 4);
    for (i = 0; i < n; i++)
        pthread_create(&prod[i], NULL, producer, bq);
    for (i = 0; i < m; i++)
        pthread_create(&cons[i], NULL, consumer, bq);
    for (i = 0; i < n; i++)
        pthread_join(prod[i], NULL);
    for (i = 0; i < m; i++)
        pthread_join(cons[i], NULL);
    blocking_queue_destroy(bq);
    free(bq);
    printf("OK\n");
}
