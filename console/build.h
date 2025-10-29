// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef BUILD_H
#define BUILD_H

#include "common.h"
#include "file_utils.h"
#include "security.h"
#include <cweb/codegen.h>
#include <cweb/autofree.h>

int cweb_compile(const char *file_path);
int handle_build_command(int argc, char *argv[]);
int combine_file_lists(file_list_t *result, size_t num_lists, ...);

extern const command_t BUILD_COMMAND;

#endif // BUILD_H
