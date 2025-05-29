#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define LINE_BUF_SIZE 8192

static const char *search_word;
static int ignore_case = 0;

// Поиск подстроки с учётом регистра
char *find_substr(const char *haystack, const char *needle) {
    if (!ignore_case)
        return strstr(haystack, needle);
    size_t nl = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        size_t i;
        for (i = 0; i < nl; ++i) {
            if (tolower((unsigned char)p[i]) != tolower((unsigned char)needle[i]))
                break;
        }
        if (i == nl)
            return (char*)p;
    }
    return NULL;
}

void search_file(const char *filepath) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) return;
    struct stat st;
    if (fstat(fd, &st) < 0 || st.st_size == 0) {
        close(fd);
        return;
    }
    char *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return;
    }

    unsigned long lineno = 1;
    size_t line_start = 0;
    for (size_t i = 0; i <= (size_t)st.st_size; ++i) {
        if (i == (size_t)st.st_size || data[i] == '\n') {
            size_t line_len = i - line_start;
            char *line = malloc(line_len + 1);
            memcpy(line, data + line_start, line_len);
            line[line_len] = '\0';
            if (find_substr(line, search_word)) {
                printf("%s:%lu: %s\n", filepath, lineno, line);
            }
            free(line);
            line_start = i + 1;
            lineno++;
        }
    }

    munmap(data, st.st_size);
    close(fd);
}

void process_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(fullpath, &st) == -1) continue;
        if (S_ISDIR(st.st_mode)) {
            process_dir(fullpath);
        } else if (S_ISREG(st.st_mode)) {
            search_file(fullpath);
        }
    }
    closedir(d);
}

int main(int argc, char *argv[]) {
    const char *dir = NULL;
    int argi = 1;
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        ignore_case = 1;
        argi++;
    }
    if (argi < argc)
        dir = argv[argi++];
    else {
        const char *home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Не задана директория и переменная HOME не установлена.\n");
            return EXIT_FAILURE;
        }
        static char default_dir[PATH_MAX];
        snprintf(default_dir, sizeof(default_dir), "%s/", home);
        dir = default_dir;
    }
    if (argi < argc)
        search_word = argv[argi];
    else {
        fprintf(stderr, "Использование: %s [-i] [директория] слово\n", argv[0]);
        return EXIT_FAILURE;
    }
    process_dir(dir);
    return EXIT_SUCCESS;
}