#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define SIZE 10000000u
#define LINE_LENGTH 256
#define STR_LENGTH 32

static uint32_t *mem;

static void allocate_first_fit(uint32_t adrs, uint32_t size) {
    if (size == 0 || size > SIZE) return;

    size_t run = 0;
    size_t start = 0;

    for (size_t i = 0; i < SIZE; ++i) {
        if (mem[i] == 0) {
            if (run == 0) start = i;
            run++;
            if (run >= size) {
                for (size_t j = start; j < start + size; ++j) mem[j] = adrs;
                return;
            }
        } else {
            run = 0;
        }
    }
}

static void allocate_best_fit(uint32_t adrs, uint32_t size) {
    if (size == 0 || size > SIZE) return;

    size_t run = 0, start = 0;
    size_t best_len = (size_t)-1;
    size_t best_start = 0;

    for (size_t i = 0; i < SIZE; ++i) {
        if (mem[i] == 0) {
            if (run == 0) start = i;
            run++;
        } else {
            if (run >= size && run < best_len) {
                best_len = run;
                best_start = start;
            }
            run = 0;
        }
    }

    if (run >= size && run < best_len) {
        best_len = run;
        best_start = start;
    }

    if (best_len == (size_t)-1) return;

    for (size_t i = best_start; i < best_start + size; ++i) mem[i] = adrs;
}

static void allocate_worst_fit(uint32_t adrs, uint32_t size) {
    if (size == 0 || size > SIZE) return;

    size_t run = 0, start = 0;
    size_t worst_len = 0;
    size_t worst_start = 0;

    for (size_t i = 0; i < SIZE; ++i) {
        if (mem[i] == 0) {
            if (run == 0) start = i;
            run++;
        } else {
            if (run >= size && run > worst_len) {
                worst_len = run;
                worst_start = start;
            }
            run = 0;
        }
    }

    if (run >= size && run > worst_len) {
        worst_len = run;
        worst_start = start;
    }

    if (worst_len == 0) return;

    for (size_t i = worst_start; i < worst_start + size; ++i) mem[i] = adrs;
}

static void clear_mem(uint32_t adrs) {
    for (size_t i = 0; i < SIZE; ++i) {
        if (mem[i] == adrs) mem[i] = 0;
    }
}

static void chomp(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static void run_algorithm(const char *title, void (*alloc_fn)(uint32_t, uint32_t), const char *filename) {
    printf("\033[1;31m%s\033[0m\n", title);

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open queries file");
        return;
    }

    char line[LINE_LENGTH];
    int query_count = 0;
    int started = 0;
    clock_t start_time = 0;

    while (fgets(line, sizeof(line), file)) {
        chomp(line);
        if (strcmp(line, "end") == 0) break;

        char cmd[STR_LENGTH];
        uint32_t adrs = 0, size = 0;

        if (sscanf(line, "%31s", cmd) != 1) continue;

        if (!started) {
            started = 1;
            start_time = clock();
        }

        if (strcmp(cmd, "allocate") == 0) {
            if (sscanf(line, "%31s %u %u", cmd, &adrs, &size) == 3) {
                alloc_fn(adrs, size);
                query_count++;
            }
        } else if (strcmp(cmd, "clear") == 0) {
            if (sscanf(line, "%31s %u", cmd, &adrs) == 2) {
                clear_mem(adrs);
                query_count++;
            }
        } else {
            printf("Invalid command: %s\n", cmd);
            fclose(file);
            return;
        }
    }

    clock_t end_time = started ? clock() : 0;
    double elapsed = started ? (double)(end_time - start_time) / CLOCKS_PER_SEC : 0.0;

    printf("Total queries executed: %d\n", query_count);
    printf("Total allocation time: %.3f seconds\n", elapsed);
    if (elapsed > 0.0) {
        printf("Throughput: %.3f queries/second\n", query_count / elapsed);
    }
    printf("\n");

    fclose(file);
}

int main(void) {
    mem = (uint32_t *)calloc(SIZE, sizeof(uint32_t));
    if (!mem) {
        perror("Failed to allocate memory");
        return 1;
    }

    const char *filename = "queries.txt";

    struct {
        const char *name;
        void (*fn)(uint32_t, uint32_t);
    } algos[] = {
        {"First fit", allocate_first_fit},
        {"Best fit",  allocate_best_fit},
        {"Worst fit", allocate_worst_fit}
    };

    for (size_t i = 0; i < sizeof(algos) / sizeof(algos[0]); ++i) {
        memset(mem, 0, SIZE * sizeof(uint32_t));
        run_algorithm(algos[i].name, algos[i].fn, filename);
    }

    free(mem);
    return 0;
}

