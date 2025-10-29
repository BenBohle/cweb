// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_PATHUTILS_H
#define CWEB_PATHUTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
#define UTILS_MAX_PATH_LEN 256

// 1. Pfad- und Query-Parsing
char* cweb_get_route_base(const char *path);
char* cweb_get_query_string(char *path);
bool cweb_has_query_param(char *path, char *key);
char* cweb_get_query_param(char *path, char *key);
int cweb_get_query_param_int(char *path, char *key, int default_value);

// 2. Pfad-Segmentierung
int cweb_split_path_segments(const char *path, char segments[][UTILS_MAX_PATH_LEN], int max_segments);
char* cweb_get_path_segment(const char *path, int index);
int cweb_path_segment_count(const char *path);

// 3. Pfad-Normalisierung und Matching
void cweb_normalize_path(char *path);
bool cweb_path_matches(char *pattern, char *path);

// 4. Utility für REST-Parameter
char* cweb_get_path_param(char *pattern, char *path, char *param_name);

// 5. URL-Decoding
void cweb_url_decode(char *dst, const char *src, size_t max_len);

int cweb_is_path_file(const char *path);
int cweb_path_isnt_textbased(const char *path);
int cweb_is_path_img(const char *path);

// 6. Query-Parameter als Map (Header ist Key-Value-Pair aus deinem Projekt)
typedef struct { char key[64]; char value[256]; } QueryParam;
int cweb_parse_query_params(char *query, QueryParam *params, int max_params);

#ifdef __cplusplus
}
#endif

#endif // CWEB_PATHUTILS_H
