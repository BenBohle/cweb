// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/routing.h>
#include <string.h>
#include <stdlib.h>
#include <cweb/logger.h>
#include <cweb/leak_detector.h>

#define MAX_ROUTES 128

static Route routes[MAX_ROUTES];
static route_handler_t fallback_handler = NULL;
static int route_count = 0;


#define MAX_ASSET_FUNCTIONS 128

static void (*asset_functions[MAX_ASSET_FUNCTIONS])(void);
static size_t asset_function_count = 0;

void cweb_set_dynamic_subpath(char *path, int set_dynamic) {
	for (int i = 0; i < route_count; i++) {
		if (strcmp(routes[i].path, path) == 0) {
			routes[i].has_dynamic_subpath = set_dynamic;
			LOG_DEBUG("ROUTING", "Set dynamic subpath for %s to %d", path, set_dynamic);
			return;
		}
	}
	LOG_WARNING("ROUTING", "Route not found for setting dynamic subpath: %s", path);
}

void cweb_set_dynamic_param(char *path, int set_dynamic) {
	for (int i = 0; i < route_count; i++) {
		if (strcmp(routes[i].path, path) == 0) {
			routes[i].has_dynamic_param = set_dynamic;
			LOG_DEBUG("ROUTING", "Set dynamic param for %s to %d", path, set_dynamic);
			return;
		}
	}
	LOG_WARNING("ROUTING", "Route not found for setting dynamic param: %s", path);
}

int cweb_rewriteRoutePath(char *current_path, char *new_path)
{
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, current_path) == 0) {
            free(routes[i].path);
            routes[i].path = strdup(new_path);
            cweb_leak_tracker_record("routes[i].path", routes[i].path, strlen(routes[i].path), true);
            LOG_DEBUG("ROUTING", "Route path rewritten from %s to %s",
                        current_path, new_path);
            return 0;
        }
    }
    return -1;
}
void cweb_register_frontend_asset_function(void (*func)(void)) {
    if (asset_function_count < MAX_ASSET_FUNCTIONS) {
        asset_functions[asset_function_count++] = func;
    } else {
        LOG_FATAL("ASSETS", "Too many asset registration functions.");
    }
}

void cweb_call_all_frontend_asset_functions() {
    for (size_t i = 0; i < asset_function_count; i++) {
        if (asset_functions[i]) {
            asset_functions[i]();
        }
    }
}

void cweb_set_fallback_handler(route_handler_t handler) {
    fallback_handler = handler;
}

void cweb_add_route(const char *path, route_handler_t handler, bool using_session) {
    LOG_DEBUG("ROUTING", "Adding route: %s (requires session: %s)", path, using_session ? "true" : "false");
    if (route_count < MAX_ROUTES) {
        routes[route_count].path = strdup(path);
        routes[route_count].handler = handler;
        routes[route_count].using_session = using_session;
		routes[route_count].has_dynamic_subpath = 0;
		routes[route_count].has_dynamic_param = 0;
        route_count++;
       
    } else {
        LOG_FATAL("ROUTING", "Maximum route limit reached. Cannot add route: %s", path);
    }
}

int has_path_subpath(const char *path) {
    if (!path || path[0] == '\0') {
        LOG_DEBUG("ROUTING", "Invalid path provided to has_path_subpath");
        return 0;
    }
    
    // Finde das erste '?' um Query-Parameter zu ignorieren
    const char *query_start = strchr(path, '?');
    
    // Finde das erste '/' 
    const char *first_slash = strchr(path, '/');
    if (!first_slash) {
        LOG_DEBUG("ROUTING", "No slash found in path: %s", path);
        return 0;
    }
    
    // Suche nach einem weiteren '/' nach dem ersten, aber stoppe bei '?'
    const char *search_start = first_slash + 1;
    const char *search_end = query_start ? query_start : path + strlen(path);
    
    // Suche nur im Pfad-Teil (vor den Query-Parametern)
    const char *second_slash = NULL;
    for (const char *p = search_start; p < search_end; p++) {
        if (*p == '/') {
            second_slash = p;
            break;
        }
    }
    
    int has_subpath = (second_slash != NULL);
    
    LOG_DEBUG("ROUTING", "Path %s %s subpath (query ignored)", path, has_subpath ? "has" : "has no");
    return has_subpath;
}

int has_path_param(const char *path) {
    if (!path || path[0] == '\0') {
        LOG_DEBUG("ROUTING", "Invalid path provided to has_path_param");
        return 0;
    }
    
    int has_param = (strchr(path, '?') != NULL);
    LOG_DEBUG("ROUTING", "Path %s %s parameters", path, has_param ? "has" : "has no");
    return has_param;
}


route_handler_t cweb_get_route_handler(const char *path, bool *using_session) {
    if (path && path[0] == '\0')
        return NULL;

    // Temporärer Speicher für den Basis-Pfad
    AUTOFREE char *base_path = cweb_get_route_base((char *)path);


    // Suche nach einer passenden Route
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, path) == 0) {
            if (using_session) {
                routes[i].using_session = *using_session;
                LOG_INFO("ROUTING", "Route requires session: %s", routes[i].using_session ? "true" : "false");
            }
            LOG_DEBUG("ROUTING", "Handler found for path: %s", path);
            return routes[i].handler;
        }
    }

	for (int i = 0; i < route_count; i++) {
		LOG_DEBUG("ROUTING", "Checking route: %s against base path: %s", routes[i].path, base_path);
        if (strcmp(routes[i].path, base_path) == 0) {
			if (routes[i].has_dynamic_subpath && has_path_subpath(path)) {
				if ((has_path_param(path) && routes[i].has_dynamic_param) || (!routes[i].has_dynamic_param && !has_path_param(path)) || !has_path_param(path)) {
					LOG_DEBUG("ROUTING", "Dynamic subpath match for route: %s with path: %s", routes[i].path, path);
					return routes[i].handler;
				}
			}
			if (routes[i].has_dynamic_param && has_path_param(path)) {
				if ((has_path_subpath(path) && routes[i].has_dynamic_subpath) || (!routes[i].has_dynamic_subpath && !has_path_subpath(path)) || !has_path_subpath(path)) {
					LOG_DEBUG("ROUTING", "Dynamic param match for route: %s with path: %s", routes[i].path, path);
					return routes[i].handler;
				}
			}
        }
    }

    // Fallback-Handler verwenden, falls keine spezifische Route gefunden wurde
    if (fallback_handler) {
        LOG_WARNING("ROUTING", "No specific handler found. Using fallback handler.");
        return fallback_handler;
    }

    LOG_ERROR("ROUTING", "No Handler found for path: %s", path);
    return NULL; // Kein Handler gefunden
}

void cweb_clear_routes() {
    for (int i = 0; i < route_count; i++) {
        LOG_DEBUG("ROUTING", "Freeing route: %s", routes[i].path);
        cweb_leak_tracker_record("routes[route_count].path", routes[i].path, strlen(routes[i].path), false);
        free(routes[i].path);
    }
    route_count = 0;
    fallback_handler = NULL;
}