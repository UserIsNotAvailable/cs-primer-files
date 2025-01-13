#include <stdio.h>
#include <pthread.h>

#define EACH_COUNT 1000000000

/** "standard" solution */
volatile int counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void *routine(void *arg) {
    for (int i = 0; i < EACH_COUNT; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// other atomic solutions below

/** solution 1 */
// _Atomic int counter = 0;
// void *thread_entry(void *arg) {
//     for (int i = 0; i < EACH_COUNT; i++)
//         counter++;
//     return NULL;
// }

/** solution 2 */
// volatile int counter = 0;
// void *thread_entry(void *arg) {
//     for (int i = 0; i < EACH_COUNT; i++)
//         __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
//     return NULL;
// }

/** solution 3 */
// volatile int counter = 0;
// void *thread_entry(void *arg) {
//     for (int i = 0; i < EACH_COUNT; i++)
//         asm volatile ("lock incl counter(%rip);");
//     return NULL;
// }

int main() {
    pthread_t p1, p2;
    pthread_create(&p1, NULL, routine, NULL);
    pthread_create(&p2, NULL, routine, NULL);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    printf("Final count: %d (expected %d)\n", counter, 2 * EACH_COUNT);
}
