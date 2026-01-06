#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define PASSWORD_LEN 8
#define PREFIX "pass:"
#define PREFIX_LEN 5
#define TOTAL_LEN (PREFIX_LEN + PASSWORD_LEN + 1)

static int write_pid_file(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("fopen(/tmp/ex1.pid)");
        return 1;
    }
    fprintf(f, "%d", getpid());
    fclose(f);
    return 0;
}

static int generate_password(char *buf, size_t buf_len) {
    if (buf_len < TOTAL_LEN) return 1;

    FILE *rnd = fopen("/dev/random", "rb");
    if (!rnd) {
        perror("fopen(/dev/random)");
        return 1;
    }

    for (size_t i = 0; i < PREFIX_LEN; ++i) buf[i] = PREFIX[i];

    size_t n = 0;
    while (n < PASSWORD_LEN) {
        int ch = fgetc(rnd);
        if (ch == EOF) {
            perror("fgetc(/dev/random)");
            fclose(rnd);
            return 1;
        }
        if (isprint((unsigned char)ch)) {
            buf[PREFIX_LEN + n] = (char)ch;
            n++;
        }
    }

    buf[PREFIX_LEN + PASSWORD_LEN] = '\0';
    fclose(rnd);
    return 0;
}

int main(void) {
    if (write_pid_file("/tmp/ex1.pid") != 0) return EXIT_FAILURE;

    char *password = mmap(NULL, TOTAL_LEN, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (password == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    if (generate_password(password, TOTAL_LEN) != 0) return EXIT_FAILURE;

    printf("%s\n", password);
    fflush(stdout);

    for (;;) pause();

    return 0;
}

