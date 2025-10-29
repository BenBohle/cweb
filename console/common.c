// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "common.h"

void cleanup_file_list(file_list_t *list) {
    if (!list) {
        return;
    }

    for (size_t i = 0; i < list->count; i++) {
        free(list->files[i]);
    }
    free(list->files);
    list->files = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool matches_alias(const char *value, const char *primary, const char *const *aliases, size_t alias_count) {
    if (!value || !primary) {
        return false;
    }

    if (strcmp(value, primary) == 0) {
        return true;
    }

    for (size_t i = 0; i < alias_count; ++i) {
        if (aliases[i] && strcmp(value, aliases[i]) == 0) {
            return true;
        }
    }

    return false;
}
