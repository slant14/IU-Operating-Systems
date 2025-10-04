#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

int counter = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void* worker(void *arg) {
    int iters = (int)(intptr_t)arg;
    for (int i = 0; i < iters; ++i) {
        pthread_mutex_lock(&m);
        counter++;                 
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

int main(void) {
    const int N = 1000000;
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, (void*)(intptr_t)N);
    pthread_create(&t2, NULL, worker, (void*)(intptr_t)N);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Expected: %d\n", 2 * N);
    printf("Actual  : %d\n", counter);

    pthread_mutex_destroy(&m);
    return 0;
}


