// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/fileserver.h>
#include "fileserver_internal.h"

const MimeMapping mime_mappings[] = {
    {".html", "text/html", 100},
    {".htm", "text/html", 100},
    {".css", "text/css", 90},
    {".js", "application/javascript", 80},
    {".json", "application/json", 70},
    {".png", "image/png", 60},
    {".jpg", "image/jpeg", 60},
    {".jpeg", "image/jpeg", 60},
    {".gif", "image/gif", 50},
    {".svg", "image/svg+xml", 70},
    {".ico", "image/x-icon", 40},
    {".pdf", "application/pdf", 30},
    {".txt", "text/plain", 20},
    {".xml", "application/xml", 30},
    {".woff", "font/woff", 85},
    {".woff2", "font/woff2", 85},
    {".ttf", "font/ttf", 85},
    {".eot", "application/vnd.ms-fontobject", 85},
    {".mp4", "video/mp4", 10},
    {".webm", "video/webm", 10},
    {".mp3", "audio/mpeg", 10},
    {".wav", "audio/wav", 10},
    {".zip", "application/zip", 5},
    {NULL, NULL, 0}
};

const char* cweb_get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    for (int i = 0; mime_mappings[i].extension; i++) {
        if (strcasecmp(ext, mime_mappings[i].extension) == 0) {
            return mime_mappings[i].mime_type;
        }
    }
    return "application/octet-stream";
}

bool is_excluded_path(const char *rel_path) {
    if (!rel_path || server_config.exclude_count == 0) return false;
    for (size_t i = 0; i < server_config.exclude_count; i++) {
        const char *pat = server_config.exclude_patterns[i];
        if (!pat || !*pat) continue;
        // fnmatch: Muster wie "/fonts/*", "/*.map", "*/.DS_Store"
        if (fnmatch(pat, rel_path, FNM_PATHNAME) == 0) {
            LOG_DEBUG("FILESERVER", "Excluded by pattern '%s': %s", pat, rel_path);
            return true;
        }
        // Komfort: Ordner-Präfix ohne Stern erlauben ("/fonts/" matcht alles darunter)
        size_t plen = strlen(pat);
        if (pat[plen-1] == '/' && strncmp(rel_path, pat, plen) == 0) {
            LOG_DEBUG("FILESERVER", "Excluded by dir prefix '%s': %s", pat, rel_path);
            return true;
        }
    }
    return false;
}

bool is_le(void) {
    uint16_t x = 0x0102;
    return *((uint8_t*)&x + 0) == 0x02; // true = LE
}

bool cweb_fileserver_is_static_file(const char *path) {
    // Check if path starts with /static/ or has a file extension
	LOG_DEBUG("FILESERVER", "Checking if static file: %s", path);
    if (strncmp(path, server_config.lookup_path, strlen(server_config.lookup_path)) != 0) return false;
    
    const char *ext = strrchr(path, '.');
    if (ext) {
        for (int i = 0; mime_mappings[i].extension; i++) {
            if (strcasecmp(ext, mime_mappings[i].extension) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool cweb_is_file_modified(const char *filepath, time_t cached_time) {
	LOG_DEBUG("FILESERVER", "Checking if file modified: %s", filepath);
    struct stat st;
    if (stat(filepath, &st) != 0) return true;
    return st.st_mtime > cached_time;
}

// TODO: makes that sense? we cant even prioritize for example pngB over pngA if pngA is requested first
int get_resource_priority(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    
    for (int i = 0; mime_mappings[i].extension; i++) {
        if (strcasecmp(ext, mime_mappings[i].extension) == 0) {
            return mime_mappings[i].priority;
        }
    }
    return 0;
}
