// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef SECURITY_H
#define SECURITY_H

#include "common.h"

int validate_path(const char *path);
int sanitize_filename(const char *filename, char *output, size_t output_size);

#endif // SECURITY_H
