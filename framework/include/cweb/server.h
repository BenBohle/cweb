// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_SERVER_H
#define CWEB_SERVER_H

#include <cweb/fileserver.h>
#include <cweb/autofree.h>
#include <cweb/http.h>
#include <cweb/routing.h>
#include <cweb/logger.h>

#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct event_base *g_event_base;
#define AUTOFREE_EVENT __attribute__((cleanup(cleanup_free_event)))

typedef enum {
    FILESYSTEM,  // Serve directly from filesystem
   	MEMORY,      // Serve from in-memory cache
    HYBRID       // Try memory first, fallback to filesystem
} Mode;

void cweb_run_server(const char *port);
void cweb_cleanup_server();

// Add a pending response to be checked later
void cweb_add_pending_response(Request *req, Response *res, struct bufferevent *bev);
void cweb_cancel_pending_responses(struct bufferevent *bev);
void cweb_send_response(struct bufferevent *bev, Request *req, Response *res);
struct event_base *cweb_get_event_base();
// Initialize the pending response checking system
void cweb_init_pending_responses(struct event_base *base);

// Cleanup pending responses
void cweb_cleanup_pending_responses();

void cweb_cleanup_free_event(struct event **ev);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_SERVER_H */