#include <cweb/fileserver.h>
#include "fileserver_internal.h"
#include <cweb/leak_detector.h>

CachedFile* cweb_find_cached_file(const char *filename) {
	LOG_DEBUG("FILESERVER", "Searching cache for file: %s", filename);
    for (int i = 0; i < cache_count; i++) {
        if (strcmp(file_cache[i].filename, filename) == 0) {
            return &file_cache[i];
        }
    }
    return NULL;
}

int cweb_load_file_to_cache(const char *filepath, const char *relative_path) {
	LOG_DEBUG("FILESERVER", "Loading file to cache: %s", filepath);
    if (cache_count >= MAX_CACHED_FILES) {
        fprintf(stderr, "File cache is full\n");
        return -1;
    }

    AUTOFREE_CLOSE_FILE FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("fopen");
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size > (long)server_config.max_file_size) {
        printf("File %s too large (%ld bytes), skipping cache\n", filepath, file_size);
        return -1;
    }

    // Get file modification time
    struct stat st;
    if (stat(filepath, &st) != 0) {
        perror("stat");
        return -1;
    }

    // Allocate memory and read file
    char *data = malloc(file_size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for file %s\n", filepath);
        return -1;
    }

    size_t bytes_read = fread(data, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        free(data);
        fprintf(stderr, "Failed to read complete file %s\n", filepath);
        return -1;
    }

    // Store in cache
    CachedFile *cached = &file_cache[cache_count];
    strncpy(cached->filename, relative_path, MAX_FILENAME - 1);
    cached->filename[MAX_FILENAME - 1] = '\0';
    strncpy(cached->mime_type, cweb_get_mime_type(relative_path), MAX_MIME_TYPE - 1);
    cached->mime_type[MAX_MIME_TYPE - 1] = '\0';
    cached->data = data;
    cached->size = file_size;
    cached->last_modified = st.st_mtime;
    cached->is_loaded = true;
    cweb_leak_tracker_record(" cached->data",  cached->data,  cached->size, true);

    cache_count++;
    printf("Cached file: %s (%zu bytes)\n", relative_path, cached->size);
    return 0;
}

int scan_directory_recursive(const char *dir_path, const char *base_path) {
    AUTOFREE_CLOSE_DIR DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip hidden files and . ..

        AUTOFREE char *full_path = NULL;
        AUTOFREE char *relative_path = NULL;
        if (asprintf(&full_path, "%s/%s", dir_path, entry->d_name) < 0) {
            LOG_ERROR("FILESERVER", "Failed to allocate path for %s", entry->d_name);
            return -1;
        }
        
        // Calculate relative path from base
        const char *rel_start = full_path + strlen(base_path);
        if (*rel_start == '/') rel_start++;
        if (asprintf(&relative_path, "/%s", rel_start) < 0) {
            LOG_ERROR("FILESERVER", "Failed to allocate relative path for %s", entry->d_name);
            return -1;
        }

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
			if (is_excluded_path(relative_path)) {
				LOG_DEBUG("FILESERVER", "Skipping excluded file: %s", relative_path);
                // ganzen Ordner skippen
                continue;
            }
            // Recursively scan subdirectory
            scan_directory_recursive(full_path, base_path);
        } else if (S_ISREG(st.st_mode)) {
			if (is_excluded_path(relative_path)) {
				LOG_DEBUG("FILESERVER", "Skipping excluded file: %s", relative_path);
                continue; // Datei skippen
            }
            // Load regular file to cache
            cweb_load_file_to_cache(full_path, relative_path);
        }
    }

    return 0;
}
