// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "common.h"
#include "security.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int create_directory_if_not_exists(const char *path);
int find_files_with_extension(const char *directory, const char *extension, file_list_t *files);
int write_sources_conf(const file_list_t *files);
int file_exists(const char *path);
int is_directory(const char *path);
int should_compile(const char *file_path);

#endif // FILE_UTILS_H
