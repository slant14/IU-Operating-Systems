#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s <message>\n", argv[0]);
        return 1;
    }

    char *msg = argv[1];
    int p2c[2];
    int c2p[2];

    if (pipe(p2c) == -1 || pipe(c2p) == -1) {
        printf("pipe error\n");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printf("fork error\n");
        return 1;
    }

    if (pid == 0) { // Child process
        close(p2c[1]);
        close(c2p[0]);

        char buf[1024];
        int n = (int)read(p2c[0], buf, sizeof(buf) - 1);
        if (n < 0) return 1;
        buf[n] = '\0';

        printf("child (%d) got: \"%s\"\n", getpid(), buf);

        int len = (int)strlen(buf);
        write(c2p[1], &len, sizeof(len));

        close(p2c[0]);
        close(c2p[1]);
        return 0;
    } else { // Parent process
        close(p2c[0]);
        close(c2p[1]);

        write(p2c[1], msg, (int)strlen(msg));
        close(p2c[1]);

        int len = -1;
        read(c2p[0], &len, sizeof(len));
        printf("parent (%d) received length: %d\n", getpid(), len);
        close(c2p[0]);

        wait(NULL);
        return 0;
    }
}


