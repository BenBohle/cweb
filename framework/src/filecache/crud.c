#include <cweb/fileserver.h>
#include "fileserver_internal.h"

int cweb_fileserver_build_cache(const char *static_dir, const char *cache_file) {
    printf("Building file cache from directory: %s\n", static_dir);

    cweb_fileserver_clear_cache();
    
    // Scan directory and load files
    int result = scan_directory_recursive(static_dir, static_dir);
    
    if (result == 0 && cache_file) {
        cweb_fileserver_save_cache(cache_file);
    }
    
    printf("File cache built: %d files loaded\n", cache_count);
    return result;
}

int cweb_fileserver_save_cache(const char *cache_file) {
    AUTOFREE_CLOSE_FILE FILE *file = fopen(cache_file, "wb");
    if (!file) {
        perror("fopen cache file for writing");
        return -1;
    }

    // Header (LE)
    const uint32_t magic = 0xCAFEBABE;
    const uint32_t version = 1;
    const uint32_t file_count = (uint32_t)cache_count;

    if (write_u32_le(file, magic) != 0 ||
        write_u32_le(file, version) != 0 ||
        write_u32_le(file, file_count) != 0) {
        perror("write header");
        return -1;
    }

    // Dateien schreiben
    for (int i = 0; i < cache_count; i++) {
        CachedFile *cached = &file_cache[i];

        uint32_t filename_len = (uint32_t)strlen(cached->filename);
        uint32_t mime_len = (uint32_t)strlen(cached->mime_type);
        uint64_t data_size = (uint64_t)cached->size;
        uint64_t last_mod = (uint64_t)cached->last_modified;

        if (filename_len >= MAX_FILENAME || mime_len >= MAX_MIME_TYPE) {
            fprintf(stderr, "fileserver_save_cache: name too long: %s\n", cached->filename);
            return -1;
        }

        if (write_u32_le(file, filename_len) != 0 ||
            write_bytes(file, cached->filename, filename_len) != 0 ||
            write_u32_le(file, mime_len) != 0 ||
            write_bytes(file, cached->mime_type, mime_len) != 0 ||
            write_u64_le(file, data_size) != 0 ||
            write_u64_le(file, last_mod) != 0 ||
            write_bytes(file, cached->data, cached->size) != 0) {
            perror("write entry");
            return -1;
        }
    }

    if (fflush(file) != 0) {
        perror("fflush cache");
        return -1;
    }
    // optional: fsync für Crash-Sicherheit
    // fsync(fileno(file));

    printf("Cache saved to %s (%d files)\n", cache_file, cache_count);
    return 0;
}

int cweb_fileserver_load_cache(const char *cache_file) {
    AUTOFREE_CLOSE_FILE FILE *file = fopen(cache_file, "rb");
    if (!file) {
        perror("fopen cache file for reading");
        return -1;
    }

    uint32_t magic, version, file_count;
    if (read_u32_le(file, &magic) != 0 || magic != 0xCAFEBABE) {
        fprintf(stderr, "Invalid cache file magic\n");
        return -1;
    }
    if (read_u32_le(file, &version) != 0 || version != 1) {
        fprintf(stderr, "Unsupported cache file version\n");
        return -1;
    }
    if (read_u32_le(file, &file_count) != 0) {
        fprintf(stderr, "Failed to read file count\n");
        return -1;
    }

    cweb_fileserver_clear_cache();

    for (uint32_t i = 0; i < file_count && i < (uint32_t)MAX_CACHED_FILES; i++) {
        uint32_t filename_len = 0, mime_len = 0;
        uint64_t data_size = 0, last_mod = 0;

        if (read_u32_le(file, &filename_len) != 0) break;
        if (filename_len >= MAX_FILENAME) break;
        if (read_bytes(file, file_cache[i].filename, filename_len) != 0) break;
        file_cache[i].filename[filename_len] = '\0';

        if (read_u32_le(file, &mime_len) != 0) break;
        if (mime_len >= MAX_MIME_TYPE) break;
        if (read_bytes(file, file_cache[i].mime_type, mime_len) != 0) break;
        file_cache[i].mime_type[mime_len] = '\0';

        if (read_u64_le(file, &data_size) != 0) break;
        if (read_u64_le(file, &last_mod) != 0) break;

        // Größenvalidierung
        if (server_config.max_file_size && data_size > server_config.max_file_size) {
            fprintf(stderr, "fileserver_load_cache: entry too large, skipping\n");
            // Überspringen der Nutzdaten
            if (fseek(file, (long)data_size, SEEK_CUR) != 0) break;
            continue;
        }
        if (data_size > SIZE_MAX) {
            fprintf(stderr, "fileserver_load_cache: entry size overflows size_t\n");
            break;
        }

        file_cache[i].data = malloc((size_t)data_size);
        if (!file_cache[i].data) {
            perror("malloc cache entry");
            break;
        }
        if (read_bytes(file, file_cache[i].data, (size_t)data_size) != 0) {
            free(file_cache[i].data);
            file_cache[i].data = NULL;
            perror("read entry data");
            break;
        }

        file_cache[i].size = (size_t)data_size;
        file_cache[i].last_modified = (time_t)last_mod;
        file_cache[i].is_loaded = true;
        cache_count++;
    }

    printf("Cache loaded from %s (%d files)\n", cache_file, cache_count);
    return 0;
}


// -1 = error, 0 = not modified, 1 = reloaded
int update_filechache(const char *path) {
	if (!initialized || !server_config.cache_file) return -1;

	CachedFile *cached = cweb_find_cached_file(path);
	
	// Check if file needs reloading (if auto_reload is enabled)
    if (server_config.auto_reload && server_config.static_dir) {
        AUTOFREE char *full_path = NULL;
        if (asprintf(&full_path, "%s%s", server_config.static_dir, path) < 0) {
            LOG_ERROR("FILESERVER", "Failed to build path for %s", path ? path : "(null)");
            return -1;
        }
        
        if (cweb_is_file_modified(full_path, cached->last_modified)) {
			LOG_DEBUG("FILESERVER", "File modified, reloading: %s", full_path);
            // File was modified, reload it
            
            // Remove from cache and reload
            free(cached->data);
            cached->is_loaded = false;
            
            if (cweb_load_file_to_cache(full_path, path) == 0) {
                // Save updated cache to disk
				cweb_fileserver_save_cache(server_config.cache_file);
				return 1;
            }
            return -1;
        }
    }
	return 0;
}