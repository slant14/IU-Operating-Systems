#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define PAGE_SIZE 8

struct PTE {
    bool valid;
    int frame;
    bool dirty;
    int referenced;
    int counter;
    unsigned char age;
};

static struct PTE *page_table;

static int num_pages;
static int num_frames;

static char **RAM;
static char **disk;

static unsigned disk_accesses;
static int algo_type;

static volatile sig_atomic_t got_sigusr1;

static void on_sigusr1(int sig) {
    (void)sig;
    got_sigusr1 = 1;
}

static void print_divider(void) {
    printf("---------------\n");
}

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

static void page_copy(char *dst, const char *src) {
    memcpy(dst, src, PAGE_SIZE);
    dst[PAGE_SIZE] = '\0';
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

static int pick_victim_random(void) {
    for (;;) {
        int p = rand() % num_pages;
        if (page_table[p].frame != -1) return p;
    }
}

static int pick_victim_nfu(void) {
    int victim = -1;
    int min_counter = INT32_MAX;

    for (int i = 0; i < num_pages; ++i) {
        if (page_table[i].valid && page_table[i].counter < min_counter) {
            min_counter = page_table[i].counter;
            victim = i;
        }
    }
    return victim;
}

static int pick_victim_aging(void) {
    int victim = -1;
    unsigned char min_age = 255;

    for (int i = 0; i < num_pages; ++i) {
        if (page_table[i].valid && page_table[i].age < min_age) {
            min_age = page_table[i].age;
            victim = i;
        }
    }
    return victim;
}

static int pick_victim(void) {
    if (algo_type == 0) return pick_victim_random();
    if (algo_type == 1) return pick_victim_nfu();
    return pick_victim_aging();
}

static void terminate(size_t map_len) {
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
        munmap(page_table, map_len);
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

            int victim_page = pick_victim();
            if (victim_page < 0) return false;

            int victim_frame = page_table[victim_page].frame;

            printf("Chose a victim page %d\n", victim_page);
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
            page_table[victim_page].counter = 0;
            page_table[victim_page].age = 0;

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
    srand((unsigned)time(NULL));
    printf("%d\n", getpid());

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_pages> <num_frames> <random|nfu|aging>\n", argv[0]);
        return EXIT_FAILURE;
    }

    num_pages = atoi(argv[1]);
    num_frames = atoi(argv[2]);
    if (num_pages <= 0 || num_frames <= 0) {
        fprintf(stderr, "Invalid num_pages/num_frames\n");
        return EXIT_FAILURE;
    }

    const char *algo = argv[3];
    if (strcmp(algo, "random") == 0) algo_type = 0;
    else if (strcmp(algo, "nfu") == 0) algo_type = 1;
    else if (strcmp(algo, "aging") == 0) algo_type = 2;
    else {
        fprintf(stderr, "Incorrect algorithm name\n");
        return EXIT_FAILURE;
    }

    printf("Selected algorithm: %s\n", algo);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigusr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        perror("sigaction(SIGUSR1)");
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
        page_table[i].counter = 0;
        page_table[i].age = 0;
    }

    printf("Initialized page table\n");
    print_page_table();
    print_divider();

    RAM = (char **)malloc((size_t)num_frames * sizeof(char *));
    if (!RAM) {
        perror("malloc(RAM)");
        close(fd);
        terminate(map_len);
    }
    for (int i = 0; i < num_frames; ++i) {
        RAM[i] = (char *)malloc((size_t)PAGE_SIZE + 1);
        if (!RAM[i]) {
            perror("malloc(RAM[i])");
            close(fd);
            terminate(map_len);
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
        terminate(map_len);
    }
    for (int i = 0; i < num_pages; ++i) {
        disk[i] = random_page();
        if (!disk[i]) {
            fprintf(stderr, "Failed to generate disk page %d\n", i);
            close(fd);
            terminate(map_len);
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
            terminate(map_len);
        }
    }
}

