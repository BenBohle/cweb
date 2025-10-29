// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include <cweb/async.h>
#include <cweb/server.h>
#include <cweb/logger.h>
#include <event2/event.h>
#include <stdlib.h>

typedef struct {
    cweb_async_cb cb;
    void *user_data;
} cweb_async_invocation;

static void cweb_async_dispatch(evutil_socket_t fd, short ev, void *arg) {
    (void)fd; (void)ev;
    cweb_async_invocation *inv = (cweb_async_invocation *)arg;
    if (inv && inv->cb) inv->cb(inv->user_data);
    free(inv);
}

int cweb_async(cweb_async_cb cb, void *user_data) {
    if (!cb) {
        LOG_WARNING("ASYNC", "cweb_async called with NULL callback");
        return -1;
    }

    struct event_base *base = cweb_get_event_base();
    if (!base) {
        LOG_ERROR("ASYNC", "No event base available");
        return -2;
    }

    cweb_async_invocation *inv = (cweb_async_invocation *)malloc(sizeof(*inv));
    if (!inv) {
        LOG_ERROR("ASYNC", "Allocation failed");
        return -3;
    }
    inv->cb = cb;
    inv->user_data = user_data;

    const struct timeval tv = {0, 0}; // next loop tick
    if (event_base_once(base, -1, EV_TIMEOUT, cweb_async_dispatch, inv, &tv) != 0) {
        LOG_ERROR("ASYNC", "event_base_once failed");
        free(inv);
        return -4;
    }
    return 0;
}