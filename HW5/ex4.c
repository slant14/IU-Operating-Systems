#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

// primality test (from ex3)
bool is_prime(int n);

// The mutex
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

// Do not modify these variables directly, use the functions on the right side.
int k = 0;
int c = 0;

// input from command line
int n = 0;

// get next prime candidate
int get_number_to_check()
{
    int ret = k;
    if (k != n)
        k++;
    return ret;
}

// increase prime counter
void increment_primes()
{
    c++;
}

// start_routine
void* check_primes(void* arg)
{
    while (1) {
        pthread_mutex_lock(&global_lock);
        int num = get_number_to_check();
        pthread_mutex_unlock(&global_lock);
        
        if (num >= n) {
            break;
        }
        
        bool prime = is_prime(num);
        
        if (prime) {
            pthread_mutex_lock(&global_lock);
            increment_primes();
            pthread_mutex_unlock(&global_lock);
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[])
{
    n = atoi(argv[1]); 
    int m = atoi(argv[2]); 
    
    pthread_t* threads = malloc(m * sizeof(pthread_t));
    if (threads == NULL) {
        fprintf(stderr, "failed to allocate memory\n");
        return 1;
    }
    
    for (int i = 0; i < m; i++) {
        if (pthread_create(&threads[i], NULL, check_primes, NULL) != 0) {
            fprintf(stderr, "failed to create %d\n", i);
            free(threads);
            return 1;
        }
    }
    
    for (int i = 0; i < m; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("number of primes in [0, %d): %d\n", n, c);
    
    free(threads);
    pthread_mutex_destroy(&global_lock);
    
    return 0;
}

bool is_prime(int n)
{
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    
    return true;
}
