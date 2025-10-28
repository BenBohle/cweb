#include <cweb/fileserver.h>
#include "fileserver_internal.h"
#include <cweb/leak_detector.h>

int cweb_default_fileserver_config(FileServerMode mode, size_t max_file_size, bool auto_reload) {
    FileServerConfig fs_config = (FileServerConfig){0};
    fs_config.static_dir = "./assets";
    fs_config.cache_file = "./build/static_cache.bin";
    fs_config.lookup_path = "/assets/";
    fs_config.mode = mode;
    fs_config.auto_reload = auto_reload;
    fs_config.max_file_size = max_file_size;

    // Excludes (Glob auf relative Pfade beginnend mit "/")
    cweb_fileserver_config_add_exclude(&fs_config, "/*.map");       // alle Source-Maps
    cweb_fileserver_config_add_exclude(&fs_config, "/*.zip");  // macOS-Dateien
    cweb_fileserver_config_add_exclude(&fs_config, "/privat/*");   // kompletter Ordner

    if (cweb_fileserver_init(&fs_config) != 0) {
        LOG_ERROR("SERVER", "Failed to initialize file server");
        cweb_fileserver_config_free_excludes(&fs_config);
        return 1;
    }

    // Konfig-Excludes wieder freigeben (intern bereits kopiert)
    cweb_fileserver_config_free_excludes(&fs_config);

    LOG_INFO("SERVER", "File server ready. All images will be served from ultra-fast cache! <3");
    return 0;
}

// Konfig-Helfer: Pattern zu cfg hinzufügen (Ownership bei cfg, wird via fileserver_config_free_excludes freigegeben)
int cweb_fileserver_config_add_exclude(FileServerConfig *cfg, const char *pattern) {
    if (!cfg || !pattern || !*pattern) return -1;
    char *dup = strdup(pattern);
    if (!dup) return -1;
    char **newarr = realloc(cfg->exclude_patterns, (cfg->exclude_count + 1) * sizeof(char*));
    if (!newarr) { 
            /* de-record the pattern before freeing on failure */
            free(dup); return -1;
    }
    cfg->exclude_patterns = newarr;
        /* record (re)allocation of the pointer array */
    cfg->exclude_patterns[cfg->exclude_count++] = dup;
    return 0;
}


// Zur Laufzeit hinzufügen (wir kopieren in server_config)
int cweb_fileserver_add_exclude(const char *pattern) {
    if (!pattern || !*pattern) return -1;
    char *dup = strdup(pattern);
    if (!dup) return -1;
        cweb_leak_tracker_record("fileserver.runtime.pattern", dup, strlen(dup) + 1, true);
    char **newarr = realloc(server_config.exclude_patterns, (server_config.exclude_count + 1) * sizeof(char*));
    if (!newarr) { free(dup); return -1; }
    server_config.exclude_patterns = newarr;
        cweb_leak_tracker_record("server_config.exclude_patterns.array", server_config.exclude_patterns, (server_config.exclude_count + 1) * sizeof(char*), true);
    server_config.exclude_patterns[server_config.exclude_count++] = dup;
    return 0;
}



int cweb_fileserver_init(FileServerConfig *config) {
    if (initialized) {
        cweb_fileserver_destroy();
    }

    // Copy configuration
    memcpy(&server_config, config, sizeof(FileServerConfig));
    if (config->static_dir) {
        server_config.static_dir = strdup(config->static_dir);
    }
    if (config->cache_file) {
            if (server_config.static_dir)
                cweb_leak_tracker_record("server_config.static_dir", server_config.static_dir, strlen(server_config.static_dir) + 1, true);
        server_config.cache_file = strdup(config->cache_file);
    }

            if (server_config.cache_file)
                cweb_leak_tracker_record("server_config.cache_file", server_config.cache_file, strlen(server_config.cache_file) + 1, true);
    // Set defaults
    if (server_config.max_file_size == 0) {
        server_config.max_file_size = 10 * 1024 * 1024; // 10MB default
    }

	if (strlen(config->lookup_path) == 0) {
		server_config.lookup_path = strdup(config->static_dir);
	} else {
		server_config.lookup_path = strdup(config->lookup_path);
    		if (server_config.lookup_path) cweb_leak_tracker_record("server_config.lookup_path", server_config.lookup_path, strlen(server_config.lookup_path) + 1, true);
	}

    		if (server_config.lookup_path) cweb_leak_tracker_record("server_config.lookup_path", server_config.lookup_path, strlen(server_config.lookup_path) + 1, true);
	if (config->exclude_count && config->exclude_patterns) {
        server_config.exclude_patterns = calloc(config->exclude_count, sizeof(char*));
        if (!server_config.exclude_patterns) return -1;
        for (size_t i = 0; i < config->exclude_count; i++) {
            server_config.exclude_patterns[i] = strdup(config->exclude_patterns[i]);
            cweb_leak_tracker_record("server_config.exclude_patterns.array", server_config.exclude_patterns, config->exclude_count * sizeof(char*), true);
            if (!server_config.exclude_patterns[i]) return -1;
        }
        server_config.exclude_count = config->exclude_count;
    } else {
        server_config.exclude_patterns = NULL;
        server_config.exclude_count = 0;
    }

    // Load or build cache based on mode
    if (server_config.mode == FILESERVER_MODE_MEMORY || server_config.mode == FILESERVER_MODE_HYBRID) {
        if (server_config.cache_file && access(server_config.cache_file, R_OK) == 0) {
            // Load existing cache
            cweb_fileserver_load_cache(server_config.cache_file);
        } else if (server_config.static_dir) {
            // Build new cache
            cweb_fileserver_build_cache(server_config.static_dir, server_config.cache_file);
        }
    }

    initialized = true;
    printf("File server initialized (mode: %d, cached files: %d)\n", server_config.mode, cache_count);
    return 0;
}
