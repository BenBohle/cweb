// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef COMMANDS_H
#define COMMANDS_H

#include "common.h"

// Command management functions
const command_t *const *get_commands(void);
void print_usage(const char *program_name);
void print_help(void);
int execute_command(const char *command_name, int argc, char *argv[]);

#endif // COMMANDS_H
