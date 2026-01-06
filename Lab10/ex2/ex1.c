#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_LEN 256

static char *base_dir;

static int path_join(char *out, size_t out_sz, const char *dir, const char *name) {
    int n = snprintf(out, out_sz, "%s/%s", dir, name);
    return (n < 0 || (size_t)n >= out_sz) ? 1 : 0;
}

static ino_t inode_of(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return (ino_t)0;
    return st.st_ino;
}

static void create_file_and_links(void) {
    const char *content = "Hello world.";
    FILE *f = fopen("myfile1.txt", "w");
    if (!f) {
        perror("fopen(myfile1.txt)");
        exit(EXIT_FAILURE);
    }
    fprintf(f, "%s\n", content);
    fclose(f);

    if (link("myfile1.txt", "myfile11.txt") != 0) perror("link(myfile11.txt)");
    if (link("myfile1.txt", "myfile12.txt") != 0) perror("link(myfile12.txt)");

    printf("Created myfile1.txt and hard links myfile11.txt, myfile12.txt\n");
}

static void find_all_hlinks(const char *source_name) {
    ino_t src_ino = inode_of(source_name);
    if (src_ino == 0) {
        perror("stat(source)");
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(base_dir);
    if (!dir) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        struct stat st;
        if (stat(entry->d_name, &st) != 0) continue;

        if (st.st_ino == src_ino) {
            char abs_path[MAX_LEN];
            if (!realpath(entry->d_name, abs_path)) continue;

            FILE *f = fopen(entry->d_name, "r");
            if (!f) {
                printf("Inode: %lu, Path: %s\n", (unsigned long)st.st_ino, abs_path);
                continue;
            }

            char content[MAX_LEN] = {0};
            fgets(content, sizeof(content), f);
            fclose(f);

            printf("Inode: %lu, Path: %s, Content: %s", (unsigned long)st.st_ino, abs_path, content);
            if (content[0] != '\0' && content[strlen(content) - 1] != '\n') printf("\n");
        }
    }

    closedir(dir);
}

static void unlink_all_except(const char *source_name) {
    ino_t src_ino = inode_of(source_name);
    if (src_ino == 0) {
        perror("stat(source)");
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(base_dir);
    if (!dir) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        struct stat st;
        if (stat(entry->d_name, &st) != 0) continue;

        if (st.st_ino == src_ino && strcmp(entry->d_name, source_name) != 0) {
            if (unlink(entry->d_name) != 0) perror("unlink");
        }
    }

    closedir(dir);

    char full[MAX_LEN];
    if (path_join(full, sizeof(full), base_dir, source_name) == 0) {
        printf("Remaining hardlink path: %s\n", full);
    }
}

static void create_sym_link(const char *target, const char *link_name) {
    if (symlink(target, link_name) != 0) perror("symlink");
    else printf("Created symlink %s -> %s\n", link_name, target);
}

static void print_single_stat(const char *file) {
    struct stat st;
    if (stat(file, &st) != 0) {
        perror("stat");
        return;
    }

    printf("================\n");
    printf("File: %s\n", file);
    printf("    Size: %ld bytes\n", (long)st.st_size);
    printf("    Change time: %s", ctime(&st.st_ctime));
    printf("    Last modification time: %s", ctime(&st.st_mtime));
    printf("    Number of hard links: %ld\n", (long)st.st_nlink);
    printf("================\n");
}

static void write_message(const char *path, const char *msg) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("open");
        return;
    }
    if (write(fd, msg, strlen(msg)) < 0) perror("write");
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    base_dir = argv[1];
    if (chdir(base_dir) != 0) {
        perror("chdir");
        return EXIT_FAILURE;
    }

    create_file_and_links();
    find_all_hlinks("myfile1.txt");

    if (rename("myfile1.txt", "/tmp/myfile1.txt") != 0) perror("rename");
    else printf("Moved myfile1.txt to /tmp/myfile1.txt\n");

    const char *msg = "Modified Hello World!";
    write_message("myfile11.txt", msg);

    create_sym_link("/tmp/myfile1.txt", "myfile13.txt");
    write_message("/tmp/myfile1.txt", msg);

    unlink_all_except("myfile11.txt");
    print_single_stat("myfile11.txt");

    return EXIT_SUCCESS;
}

