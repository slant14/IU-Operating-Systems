#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define TARGET_SIZE (500ULL * 1024ULL * 1024ULL)
#define LINE_MAX 1024

static int generate_text_file(const char *path) {
    FILE *out = fopen(path, "wb");
    if (!out) {
        perror("fopen(text.txt)");
        return 1;
    }

    FILE *rnd = fopen("/dev/random", "rb");
    if (!rnd) {
        perror("fopen(/dev/random)");
        fclose(out);
        return 1;
    }

    uint64_t written = 0;
    unsigned line_len = 0;

    while (written < TARGET_SIZE) {
        int ch = fgetc(rnd);
        if (ch == EOF) {
            perror("fgetc(/dev/random)");
            fclose(rnd);
            fclose(out);
            return 1;
        }

        if (isprint((unsigned char)ch)) {
            if (fputc(ch, out) == EOF) {
                perror("fputc(text.txt)");
                fclose(rnd);
                fclose(out);
                return 1;
            }
            written++;
            line_len++;

            if (line_len == LINE_MAX && written < TARGET_SIZE) {
                if (fputc('\n', out) == EOF) {
                    perror("fputc(newline)");
                    fclose(rnd);
                    fclose(out);
                    return 1;
                }
                written++;
                line_len = 0;
            }
        }
    }

    fclose(rnd);
    fclose(out);
    return 0;
}

static int process_file_mmap(const char *path) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        perror("sysconf(_SC_PAGESIZE)");
        return 1;
    }

    size_t chunk_size = 1024u * (size_t)page_size;

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open(text.txt)");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat(text.txt)");
        close(fd);
        return 1;
    }

    off_t file_size = st.st_size;
    uint64_t capitals = 0;

    for (off_t off = 0; off < file_size; off += (off_t)chunk_size) {
        size_t map_len = (size_t)((file_size - off) < (off_t)chunk_size ? (file_size - off) : (off_t)chunk_size);

        void *map = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);
        if (map == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }

        unsigned char *p = (unsigned char *)map;
        for (size_t i = 0; i < map_len; ++i) {
            if (isupper(p[i])) {
                capitals++;
                p[i] = (unsigned char)tolower(p[i]);
            }
        }

        if (munmap(map, map_len) != 0) {
            perror("munmap");
            close(fd);
            return 1;
        }
    }

    printf("Number of capital letters: %llu\n", (unsigned long long)capitals);
    close(fd);
    return 0;
}

int main(void) {
    const char *path = "text.txt";

    if (generate_text_file(path) != 0) return 1;
    if (process_file_mmap(path) != 0) return 1;

    return 0;
}

