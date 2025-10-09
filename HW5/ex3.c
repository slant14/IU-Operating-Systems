#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

// primality test
bool is_prime(int n) {
    if (n <= 1) return false;
    int i = 2;
    for (; i * i <= n; i++) {
        if (n % i == 0)
            return false;
    }
    return true;
}

// Primes counter in [a, b)
int primes_count(int a, int b) {
    int ret = 0;
    for (int i = a; i < b; i++)
        if (is_prime(i) != 0)
            ret++;
    return ret;
}

// argument to the start_routine of the thread
typedef struct prime_request {
    int a, b;
} prime_request;

// start_routine of the thread
void *prime_counter(void *arg) {
    // get the request from arg
    prime_request *req = (prime_request *)arg;
    
    // perform the request
    int *count = (int *)malloc(sizeof(int));
    *count = primes_count(req->a, req->b);
    
    return ((void *)count);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n> <m>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    
    if (n <= 0 || m <= 0) {
        fprintf(stderr, "n and m must be positive integers\n");
        return 1;
    }
    
    pthread_t *threads = (pthread_t *)malloc(m * sizeof(pthread_t));
    prime_request *requests = (prime_request *)malloc(m * sizeof(prime_request));
    
    int interval_size = n / m;
    
    for (int i = 0; i < m; i++) {
        requests[i].a = i * interval_size;
        
        if (i == m - 1) {
            requests[i].b = n;
        } else {
            requests[i].b = (i + 1) * interval_size;
        }
        
        pthread_create(&threads[i], NULL, prime_counter, &requests[i]);
    }
    
    int total_count = 0;
    for (int i = 0; i < m; i++) {
        void *result;
        pthread_join(threads[i], &result);
        int *count = (int *)result;
        total_count += *count;
        free(count); 
    }
    
    printf("%d\n", total_count);
    
    free(threads);
    free(requests);
    
    return 0;
}