#include <cweb/fileserver.h>
#include "fileserver_internal.h"
#include <cweb/leak_detector.h>

// Normalize incoming URL paths to cache keys so /assets/foo -> /foo etc.
static char* normalize_cache_path(const char *url_path) {
    if (!url_path) return NULL;
    size_t lookup_len = strlen(server_config.lookup_path);
    if (lookup_len > 0 && strncmp(url_path, server_config.lookup_path, lookup_len) == 0) {
        const char *rest = url_path + lookup_len;
        char *norm = NULL;
        if (asprintf(&norm, "/%s", rest ? rest : "") < 0) {
            return NULL;
        }

        return norm;
    }
    return strdup(url_path);
}


int cweb_serve_from_filesystem(const char *filepath, Response *res) {
	LOG_DEBUG("FILESERVER", "Serving from filesystem: %s", filepath);
    AUTOFREE_CLOSE_FILE FILE *file = fopen(filepath, "rb");
    if (!file) {
        return -1; // File not found
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer and read file
    char *data = malloc(file_size);
    if (!data) {
        return -1;
    }
    cweb_leak_tracker_record("sever_data", data, file_size, true);

    

    size_t bytes_read = fread(data, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        cweb_leak_tracker_record("sever_data", data, file_size, false);
        free(data);
        return -1;
    }

    const char *mime_type = cweb_get_mime_type(filepath);
    res->status_code = 200;
    res->priority = get_resource_priority(filepath);
    cweb_add_response_header(res, "Content-Type", mime_type);
	cweb_add_response_header(res, "Cache-Control", "public, max-age=31536000");
    res->body = data;
    res->body_len = file_size;
	res->state = PROCESSED;
	LOG_DEBUG("FILESERVER", "State PROCESSED file from filesystem: %s (%zu bytes)", filepath, res->body_len);

    return 0;
}

void cweb_fileserver_handle_request(Request *req, Response *res) {
    if (!initialized) {
        res->status_code = 500;
        res->body = strdup("File server not initialized");
        res->body_len = strlen(res->body);
        cweb_leak_tracker_record("res->body", res->body, res->body_len, true);
		res->state = PROCESSED;
        return;
    }

    const char *path = req->path;
    
    // Security check: prevent directory traversal
    if (strstr(path, "..") || strstr(path, "//")) {
        res->status_code = 403;
        res->body = strdup("Forbidden");
        res->body_len = strlen(res->body);
        cweb_leak_tracker_record("res->body", res->body, res->body_len, true);
		res->state = PROCESSED;
        return;
    }

	// URL -> Cache/Filename-Key normalisieren
    AUTOFREE char *cache_path = normalize_cache_path(path);
    const char *lookup_path = cache_path ? cache_path : path;
    if (cache_path && strcmp(path, lookup_path) != 0) {
        LOG_DEBUG("FILESERVER", "Normalize path: %s -> %s", path, lookup_path);
    }

    int result = -1;
    
    switch (server_config.mode) {
        case FILESERVER_MODE_MEMORY:
            result = cweb_serve_from_memory(path, res);
            break;
            
        case FILESERVER_MODE_FILESYSTEM:
            if (server_config.static_dir) {
				LOG_DEBUG("FILESERVER", "Serving from filesystem: %s", path);
                AUTOFREE char *full_path = NULL;
                if (asprintf(&full_path, "%s%s", server_config.static_dir, path) < 0) {
                    LOG_ERROR("FILESERVER", "Failed to build static path for %s", path ? path : "(null)");
                    res->status_code = 500;
                    res->body = strdup("Internal Server Error");
                    res->body_len = strlen(res->body);
                    res->state = PROCESSED;
                    return;
                }
                result = cweb_serve_from_filesystem(full_path, res);
            }
            break;
            
        case FILESERVER_MODE_HYBRID:
            // Try memory first, fallback to filesystem
            result = cweb_serve_from_memory(lookup_path, res);
            if (result != 0 && server_config.static_dir) {
                AUTOFREE char *full_path = NULL;
                if (asprintf(&full_path, "%s%s", server_config.static_dir, lookup_path) < 0) {
                    LOG_ERROR("FILESERVER", "Failed to build lookup path for %s", lookup_path);
                    res->status_code = 500;
                    res->body = strdup("Internal Server Error");
                    res->body_len = strlen(res->body);
                    res->state = PROCESSED;
                    return;
                }
                result = cweb_serve_from_filesystem(full_path, res);
            }
            break;
    }

    if (result != 0) {
        res->status_code = 404;
        res->body = strdup("File not found");
        res->body_len = strlen(res->body);
		res->state = PROCESSED;
    }
}

int cweb_serve_from_memory(const char *path, Response *res) {
    
    CachedFile *cached = cweb_find_cached_file(path);
    if (!cached || !cached->is_loaded) {
		LOG_ERROR("FILESERVER", "File not found in cache: %s", path);
        return -1; // Not found in cache
    }


	// not non-blockign at the moment. will do it later, no review about that needed now
	// update_filechache(path);

    // Serve from cache

    
    // Copy data (don't give direct pointer to avoid issues)
    res->body = malloc(cached->size);
    if (res->body) {
        memcpy(res->body, cached->data, cached->size);
        res->body_len = cached->size;
    }
	res->status_code = 200;
    res->priority = get_resource_priority(path);
    cweb_add_response_header(res, "Content-Type", cached->mime_type);
	cweb_add_response_header(res, "Cache-Control", "public, max-age=31536000");
    // cweb_add_performance_headers(res, cached->mime_type);
     cweb_leak_tracker_record("res->body", res->body, res->body_len, true);
	res->state = PROCESSED;
	LOG_DEBUG("FILESERVER", "State PROCESSED file from memory: %s (%zu bytes)", path, cached->size);
    
    return res->body ? 0 : -1;
}