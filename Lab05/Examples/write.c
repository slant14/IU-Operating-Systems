#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    const char *path = ""; // Add the absolute path of the file to create pipe
    mkfifo(path, 0666);

    int fd = open(path, O_WRONLY);
    if (fd == -1) { perror("open"); return 1; }

    const char *msg = "hello from writer\n";
    write(fd, msg, strlen(msg));
    close(fd);
    return 0;
}


