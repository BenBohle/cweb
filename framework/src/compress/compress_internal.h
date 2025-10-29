// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_COMPRESS_INTERNAL_H
#define CWEB_COMPRESS_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cweb/logger.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum {
    MINIFY_KIND_NONE = 0,
    MINIFY_KIND_HTML,
    MINIFY_KIND_CSS,
    MINIFY_KIND_JS
} MinifyKind;

extern const unsigned char kAsciiWhitespace[256];
extern const unsigned char kCssPunctuation[256];
extern const unsigned char kJsPunctuation[256];


int cweb_contains_token_ci(const char *haystack, const char *needle);
double cweb_extract_q(const char *token);
int cweb_ascii_isspace(unsigned char c);

size_t cweb_minify_css(const char *input, size_t len, char *out);
size_t cweb_minify_js(const char *input, size_t len, char *out);
size_t cweb_minify_html(const char *input, size_t len, char *out);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_COMPRESS_INTERNAL_H */