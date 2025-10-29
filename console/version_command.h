// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef VERSION_COMMAND_H
#define VERSION_COMMAND_H

#include "common.h"

int handle_version_command(int argc, char *argv[]);

extern const command_t VERSION_COMMAND;

#endif // VERSION_COMMAND_H
