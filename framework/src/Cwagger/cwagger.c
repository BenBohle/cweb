// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/cwagger.h>
#include "cwaggerdoc_page.h"
#include <string.h>

static cwagger_doc g_cwagger_doc;

static void copy_string(char *dest, size_t dest_size, const char *src) {
    if (!dest || dest_size == 0) {
        return;
    }

    if (!src) {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

void cwagger_init(const char *doc_path) {
    memset(&g_cwagger_doc, 0, sizeof(g_cwagger_doc));
    copy_string(g_cwagger_doc.doc_path, sizeof(g_cwagger_doc.doc_path), doc_path);
    cweb_add_route(doc_path, cweb_cwaggerdoc_page, false);
}

int cwagger_add(const char *method, const char *path,
                const char *short_description, const cwagger_detail *detail) {
    if (!method || !path || !short_description) {
        return -1;
    }

    if (g_cwagger_doc.endpoint_count >= CWAGGER_MAX_ENDPOINTS) {
        return -1;
    }

    cwagger_endpoint *endpoint = &g_cwagger_doc.endpoints[g_cwagger_doc.endpoint_count++];

    copy_string(endpoint->method, sizeof(endpoint->method), method);
    copy_string(endpoint->path, sizeof(endpoint->path), path);
    copy_string(endpoint->short_description, sizeof(endpoint->short_description), short_description);

    if (detail) {
        memcpy(&endpoint->detail, detail, sizeof(endpoint->detail));
    } else {
        memset(&endpoint->detail, 0, sizeof(endpoint->detail));
    }

    return 0;
}

const cwagger_doc *cwagger_get_doc(void) {
    return &g_cwagger_doc;
}

cwagger_endpoint *cwagger_get_endpoints(void) {
    return g_cwagger_doc.endpoints;
}

const cwagger_endpoint *cwagger_get_endpoint(size_t index) {
    if (index >= g_cwagger_doc.endpoint_count) {
        return NULL;
    }

    return &g_cwagger_doc.endpoints[index];
}

size_t cwagger_get_endpoint_count(void) {
    return g_cwagger_doc.endpoint_count;
}

const char *cwagger_get_doc_path(void) {
    return g_cwagger_doc.doc_path;
}