#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dupstr(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n + 1);
    return p;
}
static char *join_path(const char *base, const char *name) {
    if (strcmp(base, "/") == 0) {
        size_t n = 1 + strlen(name) + 1;
        char *p = (char *)malloc(n);
        if (!p) { perror("malloc"); exit(1); }
        snprintf(p, n, "/%s", name);
        return p;
    }
    size_t n = strlen(base) + 1 + strlen(name) + 1;
    char *p = (char *)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    snprintf(p, n, "%s/%s", base, name);
    return p;
}

struct Directory;

struct File {
    int id;
    char *name;
    int  size;
    char *data;
    struct Directory *directory;
};

struct Directory {
    char *name;
    char *path;
    struct Directory *parent;
    struct File **files;
    int nf;
    struct Directory **directories;
    int nd;
};

static int next_file_id = 1;

struct Directory *create_directory(struct Directory *parent, const char *name) {
    struct Directory *d = (struct Directory *)calloc(1, sizeof *d);
    if (!d) { perror("calloc"); exit(1); }

    d->name = dupstr(name ? name : "/");
    d->parent = parent;
    d->path = parent ? join_path(parent->path, name) : dupstr("/");

    d->files = NULL;  d->nf = 0;
    d->directories = NULL; d->nd = 0;
    return d;
}

struct File *create_file(const char *name, const char *initial_text) {
    struct File *f = (struct File *)calloc(1, sizeof *f);
    if (!f) { perror("calloc"); exit(1); }

    f->id = next_file_id++;
    f->name = dupstr(name);
    if (!initial_text) initial_text = "";
    f->size = (int)strlen(initial_text);
    f->data = (char *)malloc((size_t)f->size + 1);
    if (!f->data) { perror("malloc"); exit(1); }
    memcpy(f->data, initial_text, (size_t)f->size + 1);
    f->directory = NULL;
    return f;
}

void add_file(struct File *file, struct Directory *dir) {
    dir->files = (struct File **)realloc(dir->files, (size_t)(dir->nf + 1) * sizeof *dir->files);
    if (!dir->files) { perror("realloc"); exit(1); }
    dir->files[dir->nf++] = file;
    file->directory = dir;
}

struct Directory *add_subdir(struct Directory *parent, const char *name) {
    struct Directory *child = create_directory(parent, name);
    parent->directories = (struct Directory **)realloc(
        parent->directories, (size_t)(parent->nd + 1) * sizeof *parent->directories);
    if (!parent->directories) { perror("realloc"); exit(1); }
    parent->directories[parent->nd++] = child;
    return child;
}

void overwrite_to_file(struct File *file, const char *str) {
    if (!str) str = "";
    size_t n = strlen(str);
    char *p = (char *)realloc(file->data, n + 1);
    if (!p) { perror("realloc"); exit(1); }
    file->data = p;
    memcpy(file->data, str, n + 1);
    file->size = (int)n;
}

void append_to_file(struct File *file, const char *str) {
    if (!str) return;
    size_t n = strlen(str);
    char *p = (char *)realloc(file->data, (size_t)file->size + n + 1);
    if (!p) { perror("realloc"); exit(1); }
    file->data = p;
    memcpy(file->data + file->size, str, n + 1);
    file->size += (int)n;
}

void printp_file(const struct File *file) {
    const char *base = file->directory ? file->directory->path : "";
    if (strcmp(base, "/") == 0)
        printf("/%s\n", file->name);
    else
        printf("%s/%s\n", base, file->name);
}

int main(void) {
    struct Directory *root = create_directory(NULL, "/");
    struct Directory *home = add_subdir(root, "home");
    struct Directory *bin  = add_subdir(root, "bin");

    struct File *bash = create_file("bash", "");
    add_file(bash, bin);

    struct File *ex31 = create_file("ex3-1.c", "int printf(const char * format, ...);");
    struct File *ex32 = create_file("ex3-2.c", "//This is a comment in C language");
    add_file(ex31, home);
    add_file(ex32, home);

    append_to_file(bash, "Bourne Again Shell!!");

    printp_file(bash);
    printp_file(ex31);
    printp_file(ex32);
    printf("%s\n", bash->data);

    return 0;
}

