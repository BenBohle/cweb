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

/* Request management */
fetch_request_t *fetch_request_create(fetch_client_t *client,
                                          fetch_method_t method,
                                          const char *url) {
    if (!client || !url) {
        return NULL;
    }
    
    fetch_request_t *request = calloc(1, sizeof(fetch_request_t));
    if (!request) {
        return NULL;
    }
    
    request->client = client;
    request->method = method;
    request->url = strdup(url);
    if (!request->url) {
        free(request);
        return NULL;
    }
    
    request->curl_handle = curl_easy_init();
    if (!request->curl_handle) {
        free(request->url);
        free(request);
        return NULL;
    }
    
    /* Initialize response */
    request->response.status_code = 0;
    request->response.error = FETCH_OK;
    
    return request;
}

void fetch_request_destroy(fetch_request_t *request) {
    if (!request) return;
    
    if (request->curl_handle) {
        curl_easy_cleanup(request->curl_handle);
    }
    
    free(request->url);
    free(request->final_url);
    free(request->body);
    free(request->response_buffer);
    free(request->header_buffer);
    free(request->response.body);
    free(request->response.headers);
    
    if (request->response.json) {
        fetch_json_destroy(request->response.json);
    }
    
    fetch_header_list_free(request->headers);
    fetch_form_field_list_free(request->form_fields);
    fetch_query_param_list_free(request->query_params);
    
    free(request);
}

fetch_error_t fetch_request_set_header(fetch_request_t *request,
                                           const char *name,
                                           const char *value) {
    if (!request || !name || !value) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
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
    
    header->next = request->headers;
    request->headers = header;
    
    return FETCH_OK;
}

fetch_error_t fetch_request_set_body(fetch_request_t *request,
                                         const void *data,
                                         size_t size) {
    if (!request || (!data && size > 0)) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    free(request->body);
    request->body = NULL;
    request->body_size = 0;
    
    if (size > 0) {
        request->body = malloc(size);
        if (!request->body) {
            return FETCH_ERROR_MEMORY;
        }
        memcpy(request->body, data, size);
        request->body_size = size;
    }
    
    return FETCH_OK;
}

fetch_error_t fetch_request_set_json_body(fetch_request_t *request,
                                              const fetch_json_t *json) {
    if (!request || !json) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    char *json_string = fetch_json_to_string(json);
    if (!json_string) {
        return FETCH_ERROR_JSON;
    }
    
    fetch_error_t result = fetch_request_set_body(request, json_string, strlen(json_string));
    if (result == FETCH_OK) {
        fetch_request_set_header(request, "Content-Type", "application/json");
    }
    
    free(json_string);
    return result;
}

fetch_error_t fetch_request_add_form_field(fetch_request_t *request,
                                               const char *name,
                                               const char *value) {
    if (!request || !name || !value) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    fetch_form_field_t *field = malloc(sizeof(fetch_form_field_t));
    if (!field) {
        return FETCH_ERROR_MEMORY;
    }
    
    field->name = strdup(name);
    field->value = strdup(value);
    field->filename = NULL;
    field->content_type = NULL;
    
    if (!field->name || !field->value) {
        free(field->name);
        free(field->value);
        free(field);
        return FETCH_ERROR_MEMORY;
    }
    
    field->next = request->form_fields;
    request->form_fields = field;
    
    return FETCH_OK;
}

fetch_error_t fetch_request_add_form_file(fetch_request_t *request,
                                              const char *name,
                                              const char *filename,
                                              const char *content_type) {
    if (!request || !name || !filename) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    fetch_form_field_t *field = malloc(sizeof(fetch_form_field_t));
    if (!field) {
        return FETCH_ERROR_MEMORY;
    }
    
    field->name = strdup(name);
    field->value = NULL;
    field->filename = strdup(filename);
    field->content_type = content_type ? strdup(content_type) : NULL;
    
    if (!field->name || !field->filename || (content_type && !field->content_type)) {
        free(field->name);
        free(field->filename);
        free(field->content_type);
        free(field);
        return FETCH_ERROR_MEMORY;
    }
    
    field->next = request->form_fields;
    request->form_fields = field;
    
    return FETCH_OK;
}

fetch_error_t fetch_request_add_query_param(fetch_request_t *request,
                                                const char *name,
                                                const char *value) {
    if (!request || !name || !value) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    fetch_query_param_t *param = malloc(sizeof(fetch_query_param_t));
    if (!param) {
        return FETCH_ERROR_MEMORY;
    }
    
    param->name = strdup(name);
    param->value = strdup(value);
    
    if (!param->name || !param->value) {
        free(param->name);
        free(param->value);
        free(param);
        return FETCH_ERROR_MEMORY;
    }
    
    param->next = request->query_params;
    request->query_params = param;
    
    return FETCH_OK;
}

fetch_error_t fetch_request_set_callback(fetch_request_t *request,
                                             fetch_callback_t callback,
                                             void *user_data) {
    if (!request) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    request->callback = callback;
    request->user_data = user_data;
    
    return FETCH_OK;
}

fetch_error_t fetch_request_set_progress_callback(fetch_request_t *request,
                                                      fetch_progress_callback_t callback,
                                                      void *user_data) {
    if (!request) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    request->progress_callback = callback;
    request->user_data = user_data;
    
    return FETCH_OK;
}

fetch_error_t fetch_request_execute(fetch_request_t *request) {
    if (!request || request->is_executing) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    /* Build final URL with query parameters */
    fetch_error_t result = fetch_request_build_url(request);
    if (result != FETCH_OK) {
        return result;
    }
    
    /* Setup CURL handle */
    result = fetch_request_setup_curl(request);
    if (result != FETCH_OK) {
        return result;
    }
    
    /* Add to multi handle */
    CURLMcode mres = curl_multi_add_handle(request->client->multi_handle, request->curl_handle);
    if (mres != CURLM_OK) {
        return FETCH_ERROR_CURL;
    }
    
    request->is_executing = true;
    
    /* Kick off the transfer */
    int running_handles;
    curl_multi_socket_action(request->client->multi_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    
    return FETCH_OK;
}

fetch_error_t fetch_request_cancel(fetch_request_t *request) {
    if (!request || !request->is_executing) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    request->is_cancelled = true;
    curl_multi_remove_handle(request->client->multi_handle, request->curl_handle);
    request->is_executing = false;
    
    return FETCH_OK;
}
