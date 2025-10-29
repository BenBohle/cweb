// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/fileserver.h>
#include "fileserver_internal.h"
#include <cweb/leak_detector.h>

void cweb_fileserver_destroy() {
    if (!initialized) return;

    cweb_fileserver_clear_cache();

	// Exclude-Patterns freigeben
    if (server_config.exclude_patterns) {
        for (size_t i = 0; i < server_config.exclude_count; i++) {
            free(server_config.exclude_patterns[i]);
        }
        free(server_config.exclude_patterns);
        server_config.exclude_patterns = NULL;
        server_config.exclude_count = 0;
    }
    
    free(server_config.static_dir);
    free(server_config.cache_file);
	free(server_config.lookup_path);
    memset(&server_config, 0, sizeof(server_config));
    
    initialized = false;
}

void cweb_fileserver_clear_cache() {
    for (int i = 0; i < cache_count; i++) {
        if (file_cache[i].data) {
            cweb_leak_tracker_record("cached->data",  file_cache[i].data,  file_cache[i].size, false);
            free(file_cache[i].data);
            file_cache[i].data = NULL;
        }
        file_cache[i].is_loaded = false;
    }
    cache_count = 0;
}

void cweb_fileserver_config_free_excludes(FileServerConfig *cfg) {
    if (!cfg || !cfg->exclude_patterns) return;
    for (size_t i = 0; i < cfg->exclude_count; i++) {
        free(cfg->exclude_patterns[i]);
    }
    free(cfg->exclude_patterns);
    cfg->exclude_patterns = NULL;
    cfg->exclude_count = 0;
}