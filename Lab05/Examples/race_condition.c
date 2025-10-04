#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

int counter = 0;

void* worker(void *arg) {
    int iters = (int)(intptr_t)arg;
    for (int i = 0; i < iters; ++i) {
       ++counter; 
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
    return 0;
}


