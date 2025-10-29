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

/* Event integration with libevent */

typedef struct {
    struct event *event;
    fetch_client_t *client;
} socket_info_t;

/* Timer callback for libcurl */
void fetch_client_timer_callback(evutil_socket_t fd, short what, void *arg) {

    // TODO überarbeiten 
    (void)fd;
    (void)what;

    fetch_client_t *client = (fetch_client_t *)arg;
    int running_handles;
    
    curl_multi_socket_action(client->multi_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    fetch_check_multi_info(client);
}

/* Socket callback for libcurl */
int fetch_client_socket_callback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp) {
   
    // TODO überarbeiten
    (void)easy;
   
    fetch_client_t *client = (fetch_client_t *)userp;
    socket_info_t *socket_info = (socket_info_t *)socketp;
    
    if (what == CURL_POLL_REMOVE) {
        if (socket_info) {
            event_free(socket_info->event);
            free(socket_info);
        }
    } else {
        if (!socket_info) {
            socket_info = malloc(sizeof(socket_info_t));
            socket_info->client = client;
            socket_info->event = event_new(client->event_base, s, EV_READ | EV_WRITE | EV_PERSIST,
                                          fetch_socket_event_callback, socket_info);
            curl_multi_assign(client->multi_handle, s, socket_info);
        }
        
        short events = 0;
        if (what & CURL_POLL_IN) events |= EV_READ;
        if (what & CURL_POLL_OUT) events |= EV_WRITE;
        
        event_del(socket_info->event);
        event_assign(socket_info->event, client->event_base, s, events | EV_PERSIST,
                    fetch_socket_event_callback, socket_info);
        event_add(socket_info->event, NULL);
    }
    
    return 0;
}

/* Socket event callback */
void fetch_socket_event_callback(evutil_socket_t fd, short what, void *arg) {
    socket_info_t *socket_info = (socket_info_t *)arg;
    fetch_client_t *client = socket_info->client;
    int action = 0;
    int running_handles;
    
    if (what & EV_READ) action |= CURL_CSELECT_IN;
    if (what & EV_WRITE) action |= CURL_CSELECT_OUT;
    
    curl_multi_socket_action(client->multi_handle, fd, action, &running_handles);
    fetch_check_multi_info(client);
}

/* Check for completed transfers */
void fetch_check_multi_info(fetch_client_t *client) {
    CURLMsg *msg;
    int msgs_left;
    
    while ((msg = curl_multi_info_read(client->multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL *easy = msg->easy_handle;
            fetch_request_t *request;
            
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &request);
            
            if (request && !request->is_cancelled) {
                /* Get response information */
                curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &request->response.status_code);
                curl_easy_getinfo(easy, CURLINFO_TOTAL_TIME, &request->response.total_time);
                
                /* Set response body */
                if (request->response_buffer) {
                    request->response.body = strdup(request->response_buffer);
                    request->response.body_size = request->response_buffer_size;
                }
                
                /* Set response headers */
                if (request->header_buffer) {
                    request->response.headers = strdup(request->header_buffer);
                    request->response.headers_size = request->header_buffer_size;
                }
                
                /* Set error code */
                if (msg->data.result != CURLE_OK) {
                    request->response.error = FETCH_ERROR_CURL;
                } else {
                    request->response.error = FETCH_OK;
                }
                
                /* Call user callback */
                if (request->callback) {
                    request->callback(request, &request->response, request->user_data);
                }
            }
            
            curl_multi_remove_handle(client->multi_handle, easy);
            if (request) {
                request->is_executing = false;
            }
        }
    }
}
