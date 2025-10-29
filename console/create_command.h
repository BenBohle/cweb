// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CREATE_COMMAND_H
#define CREATE_COMMAND_H

#include "common.h"
#include "file_utils.h"
#include "security.h"
#include <cweb/template.h>
#include <cweb/autofree.h>

int handle_create_command(int argc, char *argv[]);

extern const command_t CREATE_COMMAND;

#endif // CREATE_COMMAND_H
