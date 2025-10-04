#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void* worker(void *arg) {
    (void)arg;
    pthread_t tid = pthread_self();
    for (int i = 0; i < 3; i++) {
        printf("thread %lu: hello (%d)\n", (unsigned long)tid, i);
        sleep(3);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("main: both threads finished\n");
    return 0;
}


