#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static void some_random_sht(void){
    const int N = 420;                 
    double *A = malloc(N*N*sizeof(double));
    double *B = malloc(N*N*sizeof(double));
    double *C = calloc(N*N, sizeof(double));
    if(!A||!B||!C){ perror("malloc"); exit(1); }

    for(int i=0;i<N;i++)
        for(int j=0;j<N;j++){
            A[i*N+j] = (i + j) * 0.001;
            B[i*N+j] = (i - j) * 0.002;
        }

    for(int i=0;i<N;i++)
        for(int k=0;k<N;k++){
            double aik = A[i*N+k];
            for(int j=0;j<N;j++)
                C[i*N+j] += aik * B[k*N+j];
        }

    volatile double sink = 0.0;
    for(int i=0;i<N*N;i++) sink += C[i];

    free(A); free(B); free(C);
    (void)sink;
}


int main(void){
    pid_t pid1 = fork();
    if(pid1 < 0){ perror("fork"); exit(1); }

    if(pid1 == 0){
        clock_t start = clock();
        some_random_sht();
        clock_t end = clock();
        double ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
        printf("Process PID=%d PPID=%d Time(ms)=%.2f\n", getpid(), getppid(), ms);
        _exit(0); // This exit will make sure we only have 3 processes even after running the next fork without checking if we r in parent processs or not because the first child is already exited and won't see the rest of the code!
    }

    pid_t pid2 = fork();
    if(pid2 < 0){ perror("fork"); exit(1); }

    if(pid2 == 0){
        clock_t start = clock();
        some_random_sht();
        clock_t end = clock();
        double ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
        printf("Process PID=%d PPID=%d Time(ms)=%.2f\n", getpid(), getppid(), ms);
        _exit(0);
    }

    clock_t start = clock();
    some_random_sht();
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    clock_t end = clock();
    double ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("Process PID=%d PPID=%d Time(ms)=%.2f\n", getpid(), getppid(), ms);
    return 0;
}

