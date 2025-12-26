#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>




#define MESSAGE_MAX 1024

static ssize_t read_to(int fd, char *buffer, size_t capacity) {

    size_t offset = 0;
    while (offset < capacity) {
        ssize_t r = read(fd, buffer + offset, capacity - offset);
        if (r < 0) _exit(1);
        if (r == 0) break; // EOF
        offset += (size_t) r;
    }
    return (ssize_t) offset;
}


int main(int argc, char* argv[]) {
    int fd[2];
    
    if (pipe(fd) == -1) {
        perror("Pipe creation error!\n");
        _exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed!\n");
        close(fd[0]);
        close(fd[1]);
        _exit(1);
    }

    int status = 0;

    if (pid == 0) { // child proccess (subscriber)
        close(fd[1]);
        char buffer[MESSAGE_MAX + 1];
        ssize_t msg_size = read_to(fd[0], buffer, MESSAGE_MAX);
        if (msg_size < 0) {
            perror("Error reading the message from the publisher!\n");
            close(fd[0]);
            _exit(1);
        }
        buffer[msg_size] = '\0';
        printf("The subscriber with pid: %d recieved the message with lenght: %ld, the message content is: %s\n", getpid(), msg_size, buffer);
        close(fd[0]);

    } else { // parent proccess (publisher)
        close(fd[0]);
        
        char message[MESSAGE_MAX + 2];
        fgets(message, sizeof(message), stdin);

        write(fd[1], message, (int)strlen(message));
        printf("The publisher with pid %d sent the message\n", getpid());
        close(fd[1]);

        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid error!\n");
            _exit(1);
        }
    }   

    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
