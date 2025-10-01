#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/*
    *   How to observe:
    *   gcc ex2.c -o ex2
    *   ./ex2 <NUM_OF_TIMES_FOR_LOOP> & (running in bg)
    *   pstree -p <SHELL_ID>
*/


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s n\n", argv[0]);
        return 1;
    }

    char *end;
    long n = strtol(argv[1], &end, 10);
    if (*end || n < 0) {
        fprintf(stderr, "n must be a non-negative integer\n");
        return 1;
    }

    for (long i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        printf("iter=%ld pid=%d ppid=%d fork_response=%d\n",
               i, getpid(), getppid(), (int)pid);
        fflush(stdout);
        sleep(5);
    }
    return 0;
}

