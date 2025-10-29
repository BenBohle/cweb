// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/fetch.h>
#include "cweb_fetch_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global initialization */
fetch_error_t fetch_global_init(void) {
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        return FETCH_ERROR_CURL;
    }
    return FETCH_OK;
}

void fetch_global_cleanup(void) {
    curl_global_cleanup();
}

/* Default configuration */
fetch_config_t fetch_config_default(void) {
    fetch_config_t config = {
        .user_agent = "CWeb-Fetch/1.0",
        .timeout_ms = 30000,
        .connect_timeout_ms = 10000,
        .follow_redirects = true,
        .max_redirects = 5,
        .verify_ssl = true,
        .ca_cert_path = NULL,
        .proxy_url = NULL,
        .enable_compression = true
    };
    return config;
}

/* Client management */
fetch_client_t *fetch_client_create(struct event_base *base, const fetch_config_t *config) {
    if (!base) {
        return NULL;
    }
    
    fetch_client_t *client = calloc(1, sizeof(fetch_client_t));
    if (!client) {
        return NULL;
    }
    
    client->event_base = base;
    client->multi_handle = curl_multi_init();
    if (!client->multi_handle) {
        free(client);
        return NULL;
    }
    
    /* Set up multi handle options */
    curl_multi_setopt(client->multi_handle, CURLMOPT_SOCKETFUNCTION, fetch_client_socket_callback);
    curl_multi_setopt(client->multi_handle, CURLMOPT_SOCKETDATA, client);
    curl_multi_setopt(client->multi_handle, CURLMOPT_TIMERFUNCTION, fetch_client_timer_callback);
    curl_multi_setopt(client->multi_handle, CURLMOPT_TIMERDATA, client);
    
    /* Create timer event */
    client->timer_event = evtimer_new(base, fetch_client_timer_callback, client);
    if (!client->timer_event) {
        curl_multi_cleanup(client->multi_handle);
        free(client);
        return NULL;
    }
    
    /* Copy configuration */
    if (config) {
        client->config = *config;
        /* Deep copy strings */
        if (config->user_agent) {
            client->config.user_agent = strdup(config->user_agent);
        }
        if (config->ca_cert_path) {
            client->config.ca_cert_path = strdup(config->ca_cert_path);
        }
        if (config->proxy_url) {
            client->config.proxy_url = strdup(config->proxy_url);
        }
    } else {
        client->config = fetch_config_default();
        client->config.user_agent = strdup(client->config.user_agent);
    }
    
    return client;
}

void fetch_client_destroy(fetch_client_t *client) {
    if (!client) return;
    
    if (client->timer_event) {
        event_free(client->timer_event);
    }
    
    if (client->multi_handle) {
        curl_multi_cleanup(client->multi_handle);
    }
    
    fetch_header_list_free(client->default_headers);
    
    free((void*)client->config.user_agent);
    free((void*)client->config.ca_cert_path);
    free((void*)client->config.proxy_url);
    
    free(client);
}

fetch_error_t fetch_client_set_default_header(fetch_client_t *client,
                                              const char *name,
                                              const char *value) {
    if (!client || !name || !value) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    /* Remove existing header with same name */
    fetch_client_remove_default_header(client, name);
    
    /* Add new header */
    fetch_header_t *header = malloc(sizeof(fetch_header_t));
    if (!header) {
        return FETCH_ERROR_MEMORY;
    }
    
    header->name = strdup(name);
    header->value = strdup(value);
    if (!header->name || !header->value) {
        free(header->name);
        free(header->value);
        free(header);
        return FETCH_ERROR_MEMORY;
    }
    
    header->next = client->default_headers;
    client->default_headers = header;
    
    return FETCH_OK;
}

fetch_error_t fetch_client_remove_default_header(fetch_client_t *client,
                                                 const char *name) {
    if (!client || !name) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    fetch_header_t **current = &client->default_headers;
    while (*current) {
        if (strcasecmp((*current)->name, name) == 0) {
            fetch_header_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove->name);
            free(to_remove->value);
            free(to_remove);
            return FETCH_OK;
        }
        current = &(*current)->next;
    }
    
    return FETCH_OK; /* Not found is not an error */
}

/* Error handling */
const char *fetch_error_string(fetch_error_t error) {
    switch (error) {
        case FETCH_OK: return "Success";
        case FETCH_ERROR_MEMORY: return "Memory allocation failed";
        case FETCH_ERROR_INVALID_PARAM: return "Invalid parameter";
        case FETCH_ERROR_CURL: return "CURL error";
        case FETCH_ERROR_JSON: return "JSON error";
        case FETCH_ERROR_TIMEOUT: return "Timeout";
        case FETCH_ERROR_NETWORK: return "Network error";
        default: return "Unknown error";
    }
}
