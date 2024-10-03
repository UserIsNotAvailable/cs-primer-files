#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_new_numb = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_fizz_done = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_buzz_done = PTHREAD_COND_INITIALIZER;
volatile bool new_numb = false;
volatile bool fizz_done = false;
volatile bool buzz_done = false;
volatile int n = -1;

void *fizz(void *arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        while (!new_numb)
            pthread_cond_wait(&cond_new_numb, &mutex);
        new_numb = false;
        if (n % 3 == 0)
            printf("\tfizz");
        fizz_done = true;
        pthread_cond_signal(&cond_fizz_done);
        pthread_mutex_unlock(&mutex);
    }
}

void *buzz(void *arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        while (!fizz_done)
            pthread_cond_wait(&cond_fizz_done, &mutex);
        fizz_done = false;
        if (n % 5 == 0)
            printf("\tbuzz");
        buzz_done = true;
        pthread_cond_signal(&cond_buzz_done);
        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    // start two threads, running fizz and buzz respectively
    pthread_t t1, t2;
    pthread_create(&t1, NULL, fizz, NULL);
    pthread_create(&t2, NULL, buzz, NULL);
    // every 100ms, update n randomly from the range [0, 16), indefinitely
    while (true) {
        pthread_mutex_lock(&mutex);
        while (-1 != n && !buzz_done)
            pthread_cond_wait(&cond_buzz_done, &mutex);
        buzz_done = false;
        n = rand() & 0xf;
        printf("\n%d:", n);
        new_numb = true;
        pthread_cond_signal(&cond_new_numb);
        pthread_mutex_unlock(&mutex);
        usleep(100000);
    }
}
