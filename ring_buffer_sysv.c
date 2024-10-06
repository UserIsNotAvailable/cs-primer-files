#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>

typedef struct blocking_queue {
    const void **buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    pthread_mutex_t *mutex_head;
    pthread_mutex_t *mutex_tail;
    int sem_set_id;
} blocking_queue;

void blocking_queue_init(blocking_queue *const bq, const size_t capacity) {
    bq->buffer = malloc(capacity * sizeof(void *));
    bq->capacity = capacity;
    bq->head = 0;
    bq->tail = 0;
    bq->mutex_head = malloc(sizeof(pthread_mutex_t));
    bq->mutex_tail = malloc(sizeof(pthread_mutex_t));
    bq->sem_set_id = semget(IPC_PRIVATE, 2, IPC_CREAT | IPC_EXCL | 0600);
    pthread_mutex_init(bq->mutex_head, NULL);
    pthread_mutex_init(bq->mutex_tail, NULL);
    semctl(bq->sem_set_id, 0, SETVAL, 0); // first sem set with initial value 0 for current elements
    semctl(bq->sem_set_id, 1, SETVAL, capacity); // second sem set with initial value capacity for current spaces
}

void blocking_queue_destroy(const blocking_queue *const bq) {
    semctl(bq->sem_set_id, 0, IPC_RMID);// release system resource
    pthread_mutex_destroy(bq->mutex_tail);
    pthread_mutex_destroy(bq->mutex_head);
    free(bq->mutex_tail);
    free(bq->mutex_head);
    free(bq->buffer);
}

void blocking_queue_put(blocking_queue *const bq, const void *const e) {
    struct sembuf sem_op;
    sem_op.sem_num = 1; // second sem set for current spaces
    sem_op.sem_op = -1; // it's value decreases one
    sem_op.sem_flg = 0;
    semop(bq->sem_set_id, &sem_op, 1);

    pthread_mutex_lock(bq->mutex_tail);
    bq->buffer[bq->tail] = e;
    printf("%d %p -> [%lu]\n", gettid(), e, bq->tail);
    bq->tail = (bq->tail + 1) % bq->capacity;
    pthread_mutex_unlock(bq->mutex_tail);

    sem_op.sem_num = 0; // first sem set for current elements
    sem_op.sem_op = 1; // it's value increases one
    sem_op.sem_flg = 0;
    semop(bq->sem_set_id, &sem_op, 1);
}

const void *blocking_queue_take(blocking_queue *const bq) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(bq->sem_set_id, &sem_op, 1);

    pthread_mutex_lock(bq->mutex_head);
    const void *e = bq->buffer[bq->head];
    printf("%d     [%lu] -> %p\n", gettid(), bq->head, e);
    bq->head = (bq->head + 1) % bq->capacity;
    pthread_mutex_unlock(bq->mutex_head);

    sem_op.sem_num = 1;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(bq->sem_set_id, &sem_op, 1);
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
