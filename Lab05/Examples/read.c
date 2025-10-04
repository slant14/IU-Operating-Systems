#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    const char *path = ""; // Add absolute path
    mkfifo(path, 0666);

    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }

    char buf[1024];
    int n = read(fd, buf, sizeof buf - 1);
    if (n > 0) { buf[n] = '\0'; printf("reader got: %s", buf); }
    close(fd);
    return 0;
}


