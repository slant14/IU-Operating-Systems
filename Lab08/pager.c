#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define PAGE_SIZE 8

struct PTE {
    bool valid;
    int frame;
    bool dirty;
    int referenced;
};

static struct PTE *page_table;

static int num_pages;
static int num_frames;

static char **RAM;
static char **disk;

static unsigned disk_accesses = 0;
static volatile sig_atomic_t got_sigusr1 = 0;

static void on_sigusr1(int sig) {
    (void)sig;
    got_sigusr1 = 1;
}

static void page_copy(char *dst, const char *src) {
    memcpy(dst, src, PAGE_SIZE);
    dst[PAGE_SIZE] = '\0';
}

static void print_page_table(void) {
    for (int i = 0; i < num_pages; ++i) {
        printf("Page %d ---> valid=%d, frame=%d, dirty=%d, referenced=%d\n",
               i,
               page_table[i].valid,
               page_table[i].frame,
               page_table[i].dirty,
               page_table[i].referenced);
    }
}

static void print_divider(void) {
    printf("---------------\n");
}

static void print_RAM(void) {
    printf("RAM:\n");
    for (int i = 0; i < num_frames; ++i) {
        printf("Frame %d ---> %s\n", i, RAM[i]);
    }
}

static void print_disk(void) {
    printf("Disk:\n");
    for (int i = 0; i < num_pages; ++i) {
        printf("Page %d ---> %s\n", i, disk[i]);
    }
}

static char *random_page(void) {
    char *res = (char *)malloc((size_t)PAGE_SIZE + 1);
    if (!res) return NULL;

    FILE *rnd = fopen("/dev/random", "rb");
    if (!rnd) {
        free(res);
        return NULL;
    }

    int i = 0;
    while (i < PAGE_SIZE) {
        int ch = fgetc(rnd);
        if (ch == EOF) {
            fclose(rnd);
            free(res);
            return NULL;
        }
        if (isprint((unsigned char)ch)) {
            res[i++] = (char)ch;
        }
    }
    res[PAGE_SIZE] = '\0';

    fclose(rnd);
    return res;
}

static void terminate(void) {
    printf("Total disk accesses: %u\n", disk_accesses);

    if (disk) {
        for (int i = 0; i < num_pages; ++i) free(disk[i]);
        free(disk);
    }

    if (RAM) {
        for (int i = 0; i < num_frames; ++i) free(RAM[i]);
        free(RAM);
    }

    if (page_table && page_table != MAP_FAILED) {
        munmap(page_table, (size_t)num_pages * sizeof(struct PTE));
    }

    printf("Pager is terminated\n");
    exit(0);
}

static bool handle_one_request(void) {
    for (int i = 0; i < num_pages; ++i) {
        int mmu_pid = page_table[i].referenced;
        if (mmu_pid == 0) continue;

        printf("A disk access request from MMU Process (pid=%d)\n", mmu_pid);
        printf("Page %d is referenced\n", i);

        int free_frame = -1;
        for (int j = 0; j < num_frames; ++j) {
            if (RAM[j][0] == '\0') {
                free_frame = j;
                break;
            }
        }

        if (free_frame != -1) {
            printf("We can allocate it to free frame %d\n", free_frame);
            page_table[i].valid = true;
            page_table[i].frame = free_frame;
            printf("Copy data from the disk (page=%d) to RAM (frame=%d)\n", i, free_frame);
            page_copy(RAM[free_frame], disk[i]);
            disk_accesses++;
        } else {
            printf("We do not have free frames in RAM\n");

            int victim_page = -1;
            int victim_frame = -1;

            for (;;) {
                int rp = rand() % num_pages;
                if (page_table[rp].frame != -1) {
                    victim_page = rp;
                    victim_frame = page_table[rp].frame;
                    break;
                }
            }

            printf("Chose a random victim page %d\n", victim_page);
            printf("Replace/Evict it with page %d to be allocated to frame %d\n", i, victim_frame);

            if (page_table[victim_page].dirty) {
                page_copy(disk[victim_page], RAM[victim_frame]);
                page_table[victim_page].dirty = false;
                disk_accesses++;
            }

            printf("Copy data from the disk (page=%d) to RAM (frame=%d)\n", i, victim_frame);
            page_copy(RAM[victim_frame], disk[i]);
            disk_accesses++;

            page_table[victim_page].referenced = 0;
            page_table[victim_page].frame = -1;
            page_table[victim_page].valid = false;

            page_table[i].valid = true;
            page_table[i].frame = victim_frame;
        }

        page_table[i].referenced = 0;

        print_RAM();
        print_disk();
        printf("disk accesses is %u so far\n", disk_accesses);
        printf("Resume MMU process\n");
        print_divider();
        kill(mmu_pid, SIGCONT);
        return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_pages> <num_frames>\n", argv[0]);
        return EXIT_FAILURE;
    }

    srand((unsigned)time(NULL));

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigusr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        perror("sigaction(SIGUSR1)");
        return EXIT_FAILURE;
    }

    num_pages = atoi(argv[1]);
    num_frames = atoi(argv[2]);
    if (num_pages <= 0 || num_frames <= 0) {
        fprintf(stderr, "Invalid num_pages/num_frames\n");
        return EXIT_FAILURE;
    }

    int fd = open("/tmp/ex2/pagetable", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/tmp/ex2/pagetable)");
        return EXIT_FAILURE;
    }

    size_t map_len = (size_t)num_pages * sizeof(struct PTE);
    if (ftruncate(fd, (off_t)map_len) != 0) {
        perror("ftruncate");
        close(fd);
        return EXIT_FAILURE;
    }

    page_table = (struct PTE *)mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (page_table == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_pages; ++i) {
        page_table[i].dirty = false;
        page_table[i].frame = -1;
        page_table[i].referenced = 0;
        page_table[i].valid = false;
    }

    printf("Initialized page table\n");
    print_page_table();
    print_divider();

    RAM = (char **)malloc((size_t)num_frames * sizeof(char *));
    if (!RAM) {
        perror("malloc(RAM)");
        close(fd);
        terminate();
    }
    for (int i = 0; i < num_frames; ++i) {
        RAM[i] = (char *)malloc((size_t)PAGE_SIZE + 1);
        if (!RAM[i]) {
            perror("malloc(RAM[i])");
            close(fd);
            terminate();
        }
        RAM[i][0] = '\0';
    }

    printf("Initialized RAM\n");
    print_RAM();
    print_divider();

    disk = (char **)malloc((size_t)num_pages * sizeof(char *));
    if (!disk) {
        perror("malloc(disk)");
        close(fd);
        terminate();
    }
    for (int i = 0; i < num_pages; ++i) {
        disk[i] = random_page();
        if (!disk[i]) {
            fprintf(stderr, "Failed to generate disk page %d\n", i);
            close(fd);
            terminate();
        }
    }

    printf("Initialized disk\n");
    print_disk();
    print_divider();

    for (;;) {
        pause();
        if (!got_sigusr1) continue;
        got_sigusr1 = 0;

        if (!handle_one_request()) {
            close(fd);
            terminate();
        }
    }
}

