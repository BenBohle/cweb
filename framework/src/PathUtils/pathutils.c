// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/pathutils.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <cweb/leak_detector.h>

char* cweb_get_route_base(const char *path) {
    if (!path) {
        return NULL;
    }
    
    // Finde erstes '?' für Parameter
    const char *param_start = strchr(path, '?');
    
    // Finde zweites '/' für Subpfad (überspringe das erste '/' wenn am Anfang)
    const char *first_slash = strchr(path, '/');
    const char *second_slash = NULL;
    
    if (first_slash && first_slash == path) {
        // Überspringe das erste '/' und suche nach dem zweiten
        second_slash = strchr(first_slash + 1, '/');
    }
    
    // Bestimme den Endpunkt (das was zuerst kommt: ? oder zweites /)
    const char *end_point = NULL;
    if (param_start && second_slash) {
        end_point = (param_start < second_slash) ? param_start : second_slash;
    } else if (param_start) {
        end_point = param_start;
    } else if (second_slash) {
        end_point = second_slash;
    }
    
    // Berechne Länge
    size_t len = end_point ? (size_t)(end_point - path) : strlen(path);
    
    // Allokiere Speicher
    char *out = malloc(len + 1);
    if (!out) {
        return NULL;
    }
    
    memcpy(out, path, len);
    out[len] = '\0';
    
    return out;
}

char* cweb_get_query_string(char *path) {
	if (!path) return NULL;
	char *q = strchr(path, '?');
	return q ? q + 1 : NULL;
}

static void parse_query(char *query, QueryParam *params, int max_params, int *found) {
	*found = 0;
	if (!query) return;
	char *p = query;
	while (*p && *found < max_params) {
		char *eq = strchr(p, '=');
		char *amp = strchr(p, '&');
		if (!eq) break;
		size_t klen = (size_t)(eq - p);
		size_t vlen = amp ? (size_t)(amp - eq - 1) : strlen(eq + 1);
		if (klen >= sizeof(params[*found].key)) klen = sizeof(params[*found].key) - 1;
		if (vlen >= sizeof(params[*found].value)) vlen = sizeof(params[*found].value) - 1;
		strncpy(params[*found].key, p, klen);
		params[*found].key[klen] = '\0';
		strncpy(params[*found].value, eq + 1, vlen);
		params[*found].value[vlen] = '\0';
		(*found)++;
		p = amp ? amp + 1 : eq + 1 + vlen;
	}
}

bool cweb_has_query_param(char *path, char *key) {
	char *q = cweb_get_query_string(path);
	if (!q || !key) return false;
	QueryParam params[16]; int found = 0;
	parse_query(q, params, 16, &found);
	for (int i = 0; i < found; ++i) {
		if (strcmp(params[i].key, key) == 0) return true;
	}
	return false;
}

char* cweb_get_query_param(char *path, char *key) {
	char *q = cweb_get_query_string(path);
	if (!q || !key) return NULL;
	QueryParam params[16]; int found = 0;
	parse_query(q, params, 16, &found);
	for (int i = 0; i < found; ++i) {
		if (strcmp(params[i].key, key) == 0) {
			char *out = strdup(params[i].value);
			cweb_leak_tracker_record("params[i].value", out, strlen(out), true);
			return out;
		}
	}
	return NULL;
}

int cweb_get_query_param_int(char *path, char *key, int default_value) {
	char *val = cweb_get_query_param(path, key);
	if (!val) return default_value;
	int result = atoi(val);
	cweb_leak_tracker_record("routes[route_count].path", val, strlen(val), false);
	free(val);
	return result;
}

// 2. Pfad-Segmentierung
int cweb_split_path_segments(const char *path, char segments[][UTILS_MAX_PATH_LEN], int max_segments) {
	if (!path || max_segments <= 0) return 0;
	int count = 0;
	const char *p = path;
	while (*p && count < max_segments) {
		while (*p == '/') ++p;
		if (!*p) break;
		const char *next = strchr(p, '/');
		size_t len = next ? (size_t)(next - p) : strlen(p);
		if (len >= UTILS_MAX_PATH_LEN) {
			len = UTILS_MAX_PATH_LEN - 1;
		}
		memcpy(segments[count], p, len);
		segments[count][len] = '\0';
		count++;
		p = next ? next : p + len;
	}
	return count;
}

char* cweb_get_path_segment(const char *path, int index) {
	char segments[16][UTILS_MAX_PATH_LEN];
	int n = cweb_split_path_segments(path, segments, 16);
	if (index < 0 || index >= n) return NULL;
	return strdup(segments[index]);
}

int cweb_path_segment_count(const char *path) {
	char segments[16][UTILS_MAX_PATH_LEN];
	return cweb_split_path_segments(path, segments, 16);
}

// 3. Pfad-Normalisierung und Matching
void cweb_normalize_path(char *path) {
	if (!path) return;
	char *src = path, *dst = path;
	while (*src) {
		*dst = *src;
		if (*src == '/') {
			while (*(src + 1) == '/') ++src;
		}
		++src; ++dst;
	}
	*dst = '\0';
	size_t len = strlen(path);
	if (len > 1 && path[len-1] == '/') path[len-1] = '\0';
}

bool cweb_path_matches(char *pattern, char *path) {
	if (!pattern || !path) return false;
	char pseg[16][UTILS_MAX_PATH_LEN], aseg[16][UTILS_MAX_PATH_LEN];
	int pn = cweb_split_path_segments(pattern, pseg, 16);
	int an = cweb_split_path_segments(path, aseg, 16);
	int i;
	for (i = 0; i < pn && i < an; ++i) {
		if (strcmp(pseg[i], "*") == 0) return true;
		if (pseg[i][0] == ':') continue;
		if (strcmp(pseg[i], aseg[i]) != 0) return false;
	}
	return (i == pn && i == an);
}

// 4. Utility für REST-Parameter
char* cweb_get_path_param(char *pattern, char *path, char *param_name) {
	char pseg[16][UTILS_MAX_PATH_LEN], aseg[16][UTILS_MAX_PATH_LEN];
	int pn = cweb_split_path_segments(pattern, pseg, 16);
	int an = cweb_split_path_segments(path, aseg, 16);
	for (int i = 0; i < pn && i < an; ++i) {
		if (pseg[i][0] == ':' && strcmp(pseg[i]+1, param_name) == 0) {
			char *out = strdup(aseg[i]);
			cweb_leak_tracker_record("path param", out, strlen(out), true);
			return out;
		}
	}
	return NULL;
}

	// 5. URL-Decoding
void cweb_url_decode(char *dst, const char *src, size_t max_len) {
	size_t di = 0;
	for (size_t si = 0; src[si] && di + 1 < max_len; ++si) {
		if (src[si] == '%' && isxdigit(src[si+1]) && isxdigit(src[si+2])) {
			char hex[3] = {src[si+1], src[si+2], 0};
			dst[di++] = (char)strtol(hex, NULL, 16);
			si += 2;
		} else if (src[si] == '+') {
			dst[di++] = ' ';
		} else {
			dst[di++] = src[si];
		}
	}
	dst[di] = '\0';
}

// 6. Query-Parameter als Map
int cweb_parse_query_params(char *query, QueryParam *params, int max_params) {
	int found = 0;
	parse_query(query, params, max_params, &found);
	return found;
}

int cweb_is_path_file(const char *path) {
	if (!path) return 0;
	const char *last_slash = strrchr(path, '/');
	const char *last_dot = strrchr(path, '.');
	if (last_dot && (!last_slash || last_dot > last_slash)) {
		return 1; // Hat eine Dateiendung
	}
	return 0; // Kein Dateiname
}

int path_is_textbased(const char *path) {
	if (!path) return 0;
	const char *ext = strrchr(path, '.');
	if (!ext) return 1; // keien endung also html
	ext++; // Überspringe den Punkt
	const char *text_exts[] = {"html", "htm", "css", "js", "json", "xml", "txt", "csv", "md", "yaml", "yml", NULL};
	for (int i = 0; text_exts[i]; ++i) {
		if (strcasecmp(ext, text_exts[i]) == 0) {
			return 1; // Textbasierte Datei
		}
	}
	return 0; // something else
}

int cweb_is_path_img(const char *path)
{
	if (!path) return 0;
	const char *ext = strrchr(path, '.');
	if (!ext) return 0;
	ext++; // Überspringe den Punkt
	const char *img_exts[] = {"png", "jpg", "jpeg", "gif", "bmp", "webp", "svg", NULL};
	for (int i = 0; img_exts[i]; ++i) {
		if (strcasecmp(ext, img_exts[i]) == 0) {
			return 1; // Bilddatei
		}
	}
	return 0; // Keine Bilddatei
}
