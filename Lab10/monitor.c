#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LEN 256
#define BUF_LEN (16 * 1024)

static const char *watched_path;
static int inotify_fd = -1;
static int watch_fd = -1;
static volatile sig_atomic_t stop_requested = 0;

static void on_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

static void print_single_stat(const char *file) {
    struct stat st;
    if (stat(file, &st) != 0) return;

    printf("================\n");
    printf("File: %s\n", file);
    printf("    Size: %ld bytes\n", (long)st.st_size);
    printf("    Change time: %s", ctime(&st.st_ctime));
    printf("    Last modification time: %s", ctime(&st.st_mtime));
    printf("    Number of hard links: %ld\n", (long)st.st_nlink);
    printf("================\n");
}

static void print_all_stats(void) {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        print_single_stat(entry->d_name);
    }

    closedir(dir);
}

static void handle_event(const struct inotify_event *ev) {
    if (!ev->len || ev->name[0] == '\0') return;

    if (ev->mask & IN_CREATE) {
        printf("%s created.\n", ev->name);
        print_single_stat(ev->name);
    } else if (ev->mask & IN_DELETE) {
        printf("%s deleted.\n", ev->name);
    } else if (ev->mask & IN_MODIFY) {
        printf("%s modified.\n", ev->name);
        print_single_stat(ev->name);
    } else if (ev->mask & IN_ACCESS) {
        printf("%s accessed.\n", ev->name);
        print_single_stat(ev->name);
    } else if (ev->mask & IN_OPEN) {
        printf("%s opened.\n", ev->name);
        print_single_stat(ev->name);
    } else if (ev->mask & IN_ATTRIB) {
        printf("%s metadata changed.\n", ev->name);
        print_single_stat(ev->name);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    watched_path = argv[1];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    if (chdir(watched_path) != 0) {
        perror("chdir");
        return EXIT_FAILURE;
    }

    printf("%s\n", watched_path);
    print_all_stats();

    inotify_fd = inotify_init1(0);
    if (inotify_fd < 0) {
        perror("inotify_init");
        return EXIT_FAILURE;
    }

    watch_fd = inotify_add_watch(inotify_fd, ".", IN_ACCESS | IN_CREATE | IN_DELETE | IN_MODIFY | IN_OPEN | IN_ATTRIB);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return EXIT_FAILURE;
    }

    char buffer[BUF_LEN];

    while (!stop_requested) {
        ssize_t n = read(inotify_fd, buffer, sizeof(buffer));
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read(inotify)");
            break;
        }

        size_t i = 0;
        while (i < (size_t)n) {
            struct inotify_event *ev = (struct inotify_event *)&buffer[i];
            handle_event(ev);
            i += sizeof(struct inotify_event) + ev->len;
        }
    }

    print_all_stats();

    if (watch_fd >= 0) inotify_rm_watch(inotify_fd, watch_fd);
    if (inotify_fd >= 0) close(inotify_fd);

    return 0;
}

