#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <limits.h>

struct PTE {
    bool valid;
    int frame;
    bool dirty;
    int referenced;
};

static int num_pages;
static struct PTE *page_table;

static void print_page_table(void) {
    for (int i = 0; i < num_pages; ++i) {
        printf("Page %d ---> valid=%d, frame=%d, dirty=%d, referenced=%d\n",
               i,
               page_table[i].valid,
               page_table[i].frame,
               page_table[i].dirty,
               page_table[i].referenced);
    }
    printf("\n");
}

static void print_divider(void) {
    printf("---------------\n");
}

static int parse_page_index(const char *command, int *out_index) {
    if (!command || command[0] == '\0' || command[1] == '\0') return 1;

    char *end = NULL;
    errno = 0;
    long v = strtol(command + 1, &end, 10);

    if (errno != 0 || end == command + 1 || *end != '\0') return 1;
    if (v < 0 || v > INT_MAX) return 1;

    *out_index = (int)v;
    return 0;
}

int main(int argc, char *argv[]) {
    printf("%d\n", getpid());

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_pages> <cmd...> <pager_pid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    num_pages = atoi(argv[1]);
    if (num_pages <= 0) {
        fprintf(stderr, "Invalid num_pages\n");
        return EXIT_FAILURE;
    }

    int pager_pid = atoi(argv[argc - 1]);
    if (pager_pid <= 0) {
        fprintf(stderr, "Invalid pager_pid\n");
        return EXIT_FAILURE;
    }

    int number_commands = argc - 3;

    int fd = open("/tmp/ex2/pagetable", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/tmp/ex2/pagetable)");
        return EXIT_FAILURE;
    }

    size_t map_len = (size_t)num_pages * sizeof(struct PTE);
    page_table = (struct PTE *)mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (page_table == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Initialized page table\n");
    print_page_table();

    for (int i = 0; i < number_commands; ++i) {
        const char *command = argv[2 + i];
        char access_type = command[0];

        int page_index = -1;
        if (parse_page_index(command, &page_index) != 0) {
            fprintf(stderr, "Invalid command format: %s\n", command);
            kill(pager_pid, SIGUSR1);
            munmap(page_table, map_len);
            close(fd);
            return EXIT_FAILURE;
        }

        if (page_index >= num_pages) {
            printf("Incorrect page index\n");
            kill(pager_pid, SIGUSR1);
            munmap(page_table, map_len);
            close(fd);
            return EXIT_FAILURE;
        }

        if (access_type == 'W') {
            printf("Write request for page %d\n", page_index);
            page_table[page_index].dirty = true;
        } else if (access_type == 'R') {
            printf("Read request for page %d\n", page_index);
        } else {
            fprintf(stderr, "Unknown access type: %c\n", access_type);
            kill(pager_pid, SIGUSR1);
            munmap(page_table, map_len);
            close(fd);
            return EXIT_FAILURE;
        }

        if (!page_table[page_index].valid) {
            printf("It is not a valid page --> page fault\n");
            page_table[page_index].referenced = getpid();
            printf("Ask pager to load it from disk (SIGUSR1 signal) and wait\n");
            print_divider();
            kill(pager_pid, SIGUSR1);
            raise(SIGSTOP);
            printf("MMU resumed by SIGCONT signal from pager\n");
        } else {
            printf("It is a valid page\n");
        }

        print_page_table();
    }

    printf("Done all requests\n");
    printf("MMU sends SIGUSR1 to the pager\n");
    kill(pager_pid, SIGUSR1);
    printf("MMU terminates\n");

    munmap(page_table, map_len);
    close(fd);
    return 0;
}

