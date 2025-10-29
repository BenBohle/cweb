// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "version_command.h"

int handle_version_command(int argc, char *argv[]);

static const char *const VERSION_ALIASES[] = {"--version", "-v", "-t"};

const command_t VERSION_COMMAND = {
    .name = "version",
    .description = "Show the version of the cweb framework",
    .handler = handle_version_command,
    .aliases = VERSION_ALIASES,
    .alias_count = ARRAY_SIZE(VERSION_ALIASES),
};

int handle_version_command(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("CWeb CLI v0.1.0\n");
    printf("cool stuff planned\n");
    return 0;
}
