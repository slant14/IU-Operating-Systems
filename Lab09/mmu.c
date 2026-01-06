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
    int counter;
    unsigned char age;
};

struct TLB_entry {
    int page;
    int frame;
};

static int num_pages;
static struct PTE *page_table;

static struct TLB_entry *tlb;
static int tlb_size;
static int tlb_hit;
static int tlb_miss;
static int tlb_index;

static void print_page_table(void) {
    for (int i = 0; i < num_pages; ++i) {
        printf("Page %d ---> valid=%d, frame=%d, dirty=%d, referenced=%d, counter=%d, age=%u\n",
               i,
               page_table[i].valid,
               page_table[i].frame,
               page_table[i].dirty,
               page_table[i].referenced,
               page_table[i].counter,
               (unsigned)page_table[i].age);
    }
    printf("\n");
}

static void print_divider(void) {
    printf("---------------\n");
}

static int parse_command(const char *cmd, char *out_type, int *out_page) {
    if (!cmd || !out_type || !out_page) return 1;
    if (cmd[0] != 'R' && cmd[0] != 'W') return 1;
    if (cmd[1] == '\0') return 1;

    char *end = NULL;
    errno = 0;
    long v = strtol(cmd + 1, &end, 10);
    if (errno != 0 || end == cmd + 1 || *end != '\0') return 1;
    if (v < 0 || v > INT_MAX) return 1;

    *out_type = cmd[0];
    *out_page = (int)v;
    return 0;
}

static int tlb_find(int page) {
    for (int i = 0; i < tlb_size; ++i) {
        if (tlb[i].page == page) return i;
    }
    return -1;
}

static void tlb_put(int page, int frame) {
    if (tlb_size <= 0) return;
    tlb[tlb_index].page = page;
    tlb[tlb_index].frame = frame;
    tlb_index = (tlb_index + 1) % tlb_size;
}

static void aging_tick(int accessed_page) {
    for (int j = 0; j < num_pages; ++j) {
        page_table[j].age >>= 1;
    }
    page_table[accessed_page].age |= (unsigned char)(1u << 7);
}

int main(int argc, char *argv[]) {
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

    tlb_size = num_pages / 5;
    if (tlb_size < 1) tlb_size = 1;

    tlb = (struct TLB_entry *)malloc((size_t)tlb_size * sizeof(struct TLB_entry));
    if (!tlb) {
        perror("malloc(tlb)");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < tlb_size; ++i) {
        tlb[i].page = -1;
        tlb[i].frame = -1;
    }

    int fd = open("/tmp/ex2/pagetable", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/tmp/ex2/pagetable)");
        free(tlb);
        return EXIT_FAILURE;
    }

    size_t map_len = (size_t)num_pages * sizeof(struct PTE);
    page_table = (struct PTE *)mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (page_table == MAP_FAILED) {
        perror("mmap");
        close(fd);
        free(tlb);
        return EXIT_FAILURE;
    }

    printf("Initialized page table\n");
    print_page_table();

    unsigned hits = 0;

    for (int i = 0; i < number_commands; ++i) {
        const char *cmd = argv[2 + i];

        char access_type;
        int page_index;

        if (parse_command(cmd, &access_type, &page_index) != 0) {
            fprintf(stderr, "Invalid command: %s\n", cmd);
            kill(pager_pid, SIGUSR1);
            munmap(page_table, map_len);
            close(fd);
            free(tlb);
            return EXIT_FAILURE;
        }

        if (page_index >= num_pages) {
            printf("Incorrect page index\n");
            kill(pager_pid, SIGUSR1);
            munmap(page_table, map_len);
            close(fd);
            free(tlb);
            return EXIT_FAILURE;
        }

        int idx = tlb_find(page_index);
        if (idx >= 0) {
            tlb_hit++;
        } else {
            tlb_miss++;
        }

        aging_tick(page_index);
        page_table[page_index].counter++;

        if (access_type == 'W') {
            printf("Write request for page %d\n", page_index);
            page_table[page_index].dirty = true;
        } else {
            printf("Read request for page %d\n", page_index);
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
            hits++;
            printf("It is a valid page\n");
        }

        if (idx >= 0) {
            tlb[idx].page = -1;
            tlb[idx].frame = -1;
        }

        tlb_put(page_index, page_table[page_index].frame);
        print_page_table();
    }

    printf("Done all requests\n");
    printf("MMU sends SIGUSR1 to the pager\n");
    kill(pager_pid, SIGUSR1);
    printf("MMU terminates\n");

    printf("Number of hits: %u\n", hits);
    printf("Number of misses: %u\n", (unsigned)(number_commands - (int)hits));
    printf("Total number of commands: %d\n", number_commands);
    printf("Hit ratio is: %f\n", (double)hits / (double)number_commands);

    double tlb_total = (double)tlb_miss + (double)tlb_hit;
    printf("TLB miss ratio is: %f\n", tlb_total > 0.0 ? ((double)tlb_miss / tlb_total) : 0.0);

    munmap(page_table, map_len);
    close(fd);
    free(tlb);
    return 0;
}

