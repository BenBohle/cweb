#ifndef CWEB_FETCH_INTERNAL_H
#define CWEB_FETCH_INTERNAL_H

#include <cweb/fetch.h>
#include <cweb/logger.h>
#include <event2/event.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

/* Internal structures */

typedef struct fetch_header {
    char *name;
    char *value;
    struct fetch_header *next;
} fetch_header_t;

typedef struct fetch_form_field {
    char *name;
    char *value;
    char *filename;
    char *content_type;
    struct fetch_form_field *next;
} fetch_form_field_t;

typedef struct fetch_query_param {
    char *name;
    char *value;
    struct fetch_query_param *next;
} fetch_query_param_t;

struct fetch_client {
    struct event_base *event_base;
    CURLM *multi_handle;
    struct event *timer_event;
    fetch_config_t config;
    fetch_header_t *default_headers;
    int running_handles;
};

struct fetch_request {
    fetch_client_t *client;
    CURL *curl_handle;
    fetch_method_t method;
    char *url;
    char *final_url;  /* URL with query params */
    
    /* Request data */
    fetch_header_t *headers;
    char *body;
    size_t body_size;
    fetch_form_field_t *form_fields;
    fetch_query_param_t *query_params;
    
    /* Response data */
    fetch_response_t response;
    char *response_buffer;
    size_t response_buffer_size;
    char *header_buffer;
    size_t header_buffer_size;
    
    /* Callbacks */
    fetch_callback_t callback;
    fetch_progress_callback_t progress_callback;
    void *user_data;
    
    /* State */
    bool is_executing;
    bool is_cancelled;
};

struct fetch_json {
    cJSON *cjson;
};

/* Internal function declarations */
void fetch_client_timer_callback(evutil_socket_t fd, short what, void *arg);
int fetch_client_socket_callback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp);
size_t fetch_write_callback(void *contents, size_t size, size_t nmemb, void *userp);
size_t fetch_header_callback(void *contents, size_t size, size_t nmemb, void *userp);
int fetch_progress_callback_internal(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

void fetch_header_list_free(fetch_header_t *head);
void fetch_form_field_list_free(fetch_form_field_t *head);
void fetch_query_param_list_free(fetch_query_param_t *head);

fetch_error_t fetch_request_build_url(fetch_request_t *request);
fetch_error_t fetch_request_setup_curl(fetch_request_t *request);

void fetch_socket_event_callback(evutil_socket_t fd, short what, void *arg);

void fetch_check_multi_info(fetch_client_t *client);

#endif /* CWEB_FETCH_INTERNAL_H */
