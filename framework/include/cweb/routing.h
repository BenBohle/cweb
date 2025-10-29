// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef CWEB_ROUTING_H
#define CWEB_ROUTING_H

#include <cweb/http.h>
#include <cweb/autofree.h>
#include <cweb/pathutils.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define route_handler_t as a function pointer type if not already defined
typedef void (*route_handler_t)(Request *req, Response *res);

// A route handler is a function that takes a Request and populates a Response.

// muss uberarbeiten werden, um Session-Management gsceit zu unterstutzen
typedef struct {
    char *path;
    route_handler_t handler;
    bool using_session;
	int has_dynamic_subpath;
	int has_dynamic_param;
} Route;

void cweb_set_fallback_handler(route_handler_t handler);
void cweb_set_dynamic_subpath(char *path, int set_dynamic);
void cweb_set_dynamic_param(char *path, int set_dynamic);
int cweb_rewriteRoutePath(char *current_path, char *new_path);
void cweb_add_route(const char *path, route_handler_t handler, bool requires_session);
route_handler_t cweb_get_route_handler(const char *path, bool *requires_session);
void cweb_clear_routes();

#ifdef __cplusplus
}
#endif

#endif /* CWEB_ROUTING_H */
