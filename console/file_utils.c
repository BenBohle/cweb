// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "file_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int create_directory_if_not_exists(const char *path) {
    if (validate_path(path) != 0) {
        return -1;
    }
    
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "Error: Failed to create directory '%s': %s\n", 
                    path, strerror(errno));
            return -1;
        }
        printf("Created directory: %s\n", path);
    }
    return 0;
}

int find_files_with_extension(const char *directory, const char *extension, file_list_t *result) {
    DIR *dir = opendir(directory);
    if (!dir) {
        fprintf(stderr, "Error: Could not open directory '%s'\n", directory);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build full path
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        struct stat path_stat;
        if (stat(full_path, &path_stat) != 0) {
            fprintf(stderr, "Error: Could not stat '%s'\n", full_path);
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            // Rekursively search in subdirectory
            if (find_files_with_extension(full_path, extension, result) != 0) {
                closedir(dir);
                return -1;
            }
        } else if (S_ISREG(path_stat.st_mode)) {
            // Check if file has the desired extension
            const char *dot = strrchr(entry->d_name, '.');
            if (dot && strcmp(dot, extension) == 0) {
                // Add file to result list
                if (result->count >= result->capacity) {
                    size_t new_capacity = result->capacity == 0 ? 16 : result->capacity * 2;
                    char **new_files = realloc(result->files, new_capacity * sizeof(char *));
                    if (!new_files) {
                        fprintf(stderr, "Error: Memory allocation failed\n");
                        closedir(dir);
                        return -1;
                    }
                    result->files = new_files;
                    result->capacity = new_capacity;
                }

                result->files[result->count] = strdup(full_path);
                if (!result->files[result->count]) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    closedir(dir);
                    return -1;
                }
                result->count++;
            }
        }
    }

    closedir(dir);
    return 0;
}

int write_sources_conf(const file_list_t *files) {
    FILE *conf_file = fopen("build/sources.conf", "w");
    if (!conf_file) {
        fprintf(stderr, "Error: Cannot create sources.conf: %s\n", strerror(errno));
        return -1;
    }
    
    for (size_t i = 0; i < files->count; i++) {
        fprintf(conf_file, "%s\n", files->files[i]);
    }
    
    printf("Generated sources.conf with %zu files\n", files->count);
	fclose(conf_file);
    return 0;
}

int file_exists(const char *path) {
    if (validate_path(path) != 0) {
        return 0;
    }
    
    struct stat st;
    return (stat(path, &st) == 0);
}

int is_directory(const char *path) {
    if (validate_path(path) != 0) {
        return 0;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    return S_ISDIR(st.st_mode);
}