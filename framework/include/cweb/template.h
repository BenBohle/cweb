// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_TEMPLATE_H
#define CWEB_TEMPLATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <cweb/logger.h>

#ifdef __cplusplus
extern "C" {
#endif

// Buffer for output accumulation
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} cweb_buffer_t;

// Global output buffer
extern cweb_buffer_t g_output_buffer;

void cweb_output_init(void);
void cweb_output_raw(const char *str);
void cweb_output_html(const char *str);
char* cweb_output_get(void);
void cweb_output_cleanup(void);

void cweb_buffer_init(cweb_buffer_t *buffer);
void cweb_buffer_append(cweb_buffer_t *buffer, const char *str);
void cweb_buffer_cleanup(cweb_buffer_t *buffer);

bool cweb_setAssetLink(char **html,
                  const char *logical_name,     // z.B. "styles.css"
                  const char *resolved_path,    // z.B. "/home/styles.css"
                  const char *rel_attr);   

#ifdef __cplusplus
}
#endif

#endif /* CWEB_TEMPLATE_H */
