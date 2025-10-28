#ifndef COMMON_H
#define COMMON_H

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cweb/autofree.h>


#define MAX_PATH_LEN 4096
#define MAX_COMMAND_LEN 256
#define MAX_FILES 1000
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
    char **files;
    size_t count;
    size_t capacity;
} file_list_t;

typedef int (*command_handler_t)(int argc, char *argv[]);

typedef struct {
    const char *name;
    const char *description;
    command_handler_t handler;
    const char *const *aliases;
    size_t alias_count;
} command_t;

void cleanup_file_list(file_list_t *list);
bool matches_alias(const char *value, const char *primary, const char *const *aliases, size_t alias_count);

#endif // COMMON_H
