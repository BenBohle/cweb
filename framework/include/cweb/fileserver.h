// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_FILESERVER_H
#define CWEB_FILESERVER_H

#include <cweb/http.h>
#include <cweb/logger.h>
#include <cweb/autofree.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <fnmatch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILENAME 256
#define MAX_MIME_TYPE 64
#define MAX_CACHED_FILES 1024

typedef struct {
    char filename[MAX_FILENAME];
    char mime_type[MAX_MIME_TYPE];
    char *data;
    size_t size;
    time_t last_modified;
    bool is_loaded;
} CachedFile;

// File serving modes
typedef enum {
    FILESERVER_MODE_FILESYSTEM,  // Serve directly from filesystem
    FILESERVER_MODE_MEMORY,      // Serve from in-memory cache
    FILESERVER_MODE_HYBRID       // Try memory first, fallback to filesystem
} FileServerMode;

// File server configuration
typedef struct {
    char *static_dir;           // Directory containing static files
    char *cache_file;           // Path to binary cache file
	char *lookup_path;        	// Optional path normalization
    FileServerMode mode;        // Serving mode
    bool auto_reload;           // Auto-reload files when changed
    size_t max_file_size;       // Maximum file size to cache (bytes)

    char **exclude_patterns;
    size_t exclude_count;
} FileServerConfig;

/* File server initialization and cleanup */
int cweb_fileserver_init(FileServerConfig *config);
int cweb_default_fileserver_config(FileServerMode mode, size_t max_file_size, bool auto_reload);
void cweb_fileserver_destroy(void);

/* Configuration helpers */
int cweb_fileserver_config_add_exclude(FileServerConfig *cfg, const char *pattern);
void cweb_fileserver_config_free_excludes(FileServerConfig *cfg);
int cweb_fileserver_add_exclude(const char *pattern);

/* Cache management */
int cweb_fileserver_build_cache(const char *static_dir, const char *cache_file);
int cweb_fileserver_load_cache(const char *cache_file);
int cweb_fileserver_save_cache(const char *cache_file);
void cweb_fileserver_clear_cache(void);

/* Lookup helpers */
CachedFile* cweb_find_cached_file(const char *filename);
int cweb_load_file_to_cache(const char *filepath, const char *relative_path);

/* File serving */
void cweb_fileserver_handle_request(Request *req, Response *res);
bool cweb_fileserver_is_static_file(const char *path);
int cweb_serve_from_filesystem(const char *filepath, Response *res);
int cweb_serve_from_memory(const char *path, Response *res);

/* Utility functions (mostly used internally, but exposed for advanced cases) */
const char* cweb_get_mime_type(const char *filename);
bool cweb_is_file_modified(const char *filepath, time_t cached_time);
int write_bytes(FILE *f, const void *buf, size_t len);
int read_bytes(FILE *f, void *buf, size_t len);
int write_u32_le(FILE *f, uint32_t v);
int write_u64_le(FILE *f, uint64_t v);
int read_u32_le(FILE *f, uint32_t *out);
int read_u64_le(FILE *f, uint64_t *out);

/* Legacy wrappers kept for older code paths (no-op if unused). */
static inline int fileserver_init(FileServerConfig *config) {
    return cweb_fileserver_init(config);
}

static inline int default_fileserver_config(FileServerMode mode, size_t max_file_size, bool auto_reload) {
    return cweb_default_fileserver_config(mode, max_file_size, auto_reload);
}

static inline void fileserver_destroy(void) {
    cweb_fileserver_destroy();
}

static inline int fileserver_config_add_exclude(FileServerConfig *cfg, const char *pattern) {
    return cweb_fileserver_config_add_exclude(cfg, pattern);
}

static inline void fileserver_config_free_excludes(FileServerConfig *cfg) {
    cweb_fileserver_config_free_excludes(cfg);
}

static inline int fileserver_add_exclude(const char *pattern) {
    return cweb_fileserver_add_exclude(pattern);
}

static inline int fileserver_build_cache(const char *static_dir, const char *cache_file) {
    return cweb_fileserver_build_cache(static_dir, cache_file);
}

static inline int fileserver_load_cache(const char *cache_file) {
    return cweb_fileserver_load_cache(cache_file);
}

static inline int fileserver_save_cache(const char *cache_file) {
    return cweb_fileserver_save_cache(cache_file);
}

static inline void fileserver_clear_cache(void) {
    cweb_fileserver_clear_cache();
}

static inline CachedFile* find_cached_file(const char *filename) {
    return cweb_find_cached_file(filename);
}

static inline int load_file_to_cache(const char *filepath, const char *relative_path) {
    return cweb_load_file_to_cache(filepath, relative_path);
}

static inline void fileserver_handle_request(Request *req, Response *res) {
    cweb_fileserver_handle_request(req, res);
}

static inline bool fileserver_is_static_file(const char *path) {
    return cweb_fileserver_is_static_file(path);
}

static inline int serve_from_filesystem(const char *filepath, Response *res) {
    return cweb_serve_from_filesystem(filepath, res);
}

static inline int serve_from_memory(const char *path, Response *res) {
    return cweb_serve_from_memory(path, res);
}

static inline const char* get_mime_type(const char *filename) {
    return cweb_get_mime_type(filename);
}

static inline bool is_file_modified(const char *filepath, time_t cached_time) {
    return cweb_is_file_modified(filepath, cached_time);
}

#ifdef __cplusplus
}
#endif

#endif /* CWEB_FILESERVER_H */
