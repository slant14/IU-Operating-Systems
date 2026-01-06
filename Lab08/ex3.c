#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#define N_SECONDS 10
#define ALLOC_SIZE (10u * 1024u * 1024u)

static void print_maxrss_kb(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        perror("getrusage");
        return;
    }
    printf("Memory usage (ru_maxrss): %ld\n", usage.ru_maxrss);
    fflush(stdout);
}

int main(void) {
    char *blocks[N_SECONDS] = {0};

    for (int i = 0; i < N_SECONDS; ++i) {
        blocks[i] = (char *)malloc((size_t)ALLOC_SIZE);
        if (!blocks[i]) {
            perror("malloc");
            break;
        }

        memset(blocks[i], 0, (size_t)ALLOC_SIZE);
        print_maxrss_kb();
        sleep(1);
    }

    for (int i = 0; i < N_SECONDS; ++i) {
        free(blocks[i]);
    }

    return 0;
}

