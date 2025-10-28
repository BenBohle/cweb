#include <cweb/fetch.h>
#include "cweb_fetch_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Utility functions */

/* URL encoding */
char *fetch_url_encode(const char *str) {
    if (!str) return NULL;
    
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    
    char *encoded = curl_easy_escape(curl, str, 0);
    char *result = encoded ? strdup(encoded) : NULL;
    
    curl_free(encoded);
    curl_easy_cleanup(curl);
    
    return result;
}

char *fetch_url_decode(const char *str) {
    if (!str) return NULL;
    
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    
    int outlength;
    char *decoded = curl_easy_unescape(curl, str, 0, &outlength);
    char *result = decoded ? strdup(decoded) : NULL;
    
    curl_free(decoded);
    curl_easy_cleanup(curl);
    
    return result;
}

/* Helper functions for linked lists */
void fetch_header_list_free(fetch_header_t *head) {
    while (head) {
        fetch_header_t *next = head->next;
        free(head->name);
        free(head->value);
        free(head);
        head = next;
    }
}

void fetch_form_field_list_free(fetch_form_field_t *head) {
    while (head) {
        fetch_form_field_t *next = head->next;
        free(head->name);
        free(head->value);
        free(head->filename);
        free(head->content_type);
        free(head);
        head = next;
    }
}

void fetch_query_param_list_free(fetch_query_param_t *head) {
    while (head) {
        fetch_query_param_t *next = head->next;
        free(head->name);
        free(head->value);
        free(head);
        head = next;
    }
}

/* Build final URL with query parameters */
fetch_error_t fetch_request_build_url(fetch_request_t *request) {
    if (!request->query_params) {
        request->final_url = strdup(request->url);
        return request->final_url ? FETCH_OK : FETCH_ERROR_MEMORY;
    }
    
    /* Calculate required buffer size */
    size_t url_len = strlen(request->url);
    size_t params_len = 0;
    
    fetch_query_param_t *param = request->query_params;
    while (param) {
        char *encoded_name = fetch_url_encode(param->name);
        char *encoded_value = fetch_url_encode(param->value);
        
        if (encoded_name && encoded_value) {
            params_len += strlen(encoded_name) + strlen(encoded_value) + 2; /* = and & */
        }
        
        free(encoded_name);
        free(encoded_value);
        param = param->next;
    }
    
    /* Allocate buffer */
    request->final_url = malloc(url_len + params_len + 2); /* ? and \0 */
    if (!request->final_url) {
        return FETCH_ERROR_MEMORY;
    }
    
    /* Build URL */
    strcpy(request->final_url, request->url);
    strcat(request->final_url, "?");
    
    param = request->query_params;
    bool first = true;
    while (param) {
        if (!first) {
            strcat(request->final_url, "&");
        }
        
        char *encoded_name = fetch_url_encode(param->name);
        char *encoded_value = fetch_url_encode(param->value);
        
        if (encoded_name && encoded_value) {
            strcat(request->final_url, encoded_name);
            strcat(request->final_url, "=");
            strcat(request->final_url, encoded_value);
            first = false;
        }
        
        free(encoded_name);
        free(encoded_value);
        param = param->next;
    }
    
    return FETCH_OK;
}

/* Setup CURL handle for request */
fetch_error_t fetch_request_setup_curl(fetch_request_t *request) {
    CURL *curl = request->curl_handle;
    fetch_client_t *client = request->client;
    
    /* Basic options */
    curl_easy_setopt(curl, CURLOPT_URL, request->final_url ? request->final_url : request->url);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, request);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, client->config.user_agent);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, client->config.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, client->config.connect_timeout_ms);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, client->config.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, client->config.max_redirects);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, client->config.verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, client->config.verify_ssl ? 2L : 0L);
    
    if (client->config.ca_cert_path) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, client->config.ca_cert_path);
    }
    
    if (client->config.proxy_url) {
        curl_easy_setopt(curl, CURLOPT_PROXY, client->config.proxy_url);
    }
    
    if (client->config.enable_compression) {
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    }
    
    /* Set HTTP method */
    switch (request->method) {
        case FETCH_GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case FETCH_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            break;
        case FETCH_PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;
        case FETCH_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case FETCH_PATCH:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            break;
        case FETCH_HEAD:
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            break;
        case FETCH_OPTIONS:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
            break;
    }
    
    /* Set callbacks */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, fetch_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, request);
    
    if (request->progress_callback) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, fetch_progress_callback_internal);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, request);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    
    /* Set request body */
    if (request->body && request->body_size > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request->body_size);
    }
    
    /* Build headers */
    struct curl_slist *headers = NULL;
    
    /* Add default headers */
    fetch_header_t *header = client->default_headers;
    while (header) {
        char *header_line = malloc(strlen(header->name) + strlen(header->value) + 3);
        if (header_line) {
            sprintf(header_line, "%s: %s", header->name, header->value);
            headers = curl_slist_append(headers, header_line);
            free(header_line);
        }
        header = header->next;
    }
    
    /* Add request-specific headers */
    header = request->headers;
    while (header) {
        char *header_line = malloc(strlen(header->name) + strlen(header->value) + 3);
        if (header_line) {
            sprintf(header_line, "%s: %s", header->name, header->value);
            headers = curl_slist_append(headers, header_line);
            free(header_line);
        }
        header = header->next;
    }
    
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    return FETCH_OK;
}

/* CURL callbacks */
size_t fetch_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    fetch_request_t *request = (fetch_request_t *)userp;
    size_t realsize = size * nmemb;
    
    char *ptr = realloc(request->response_buffer, request->response_buffer_size + realsize + 1);
    if (!ptr) {
        return 0; /* Out of memory */
    }
    
    request->response_buffer = ptr;
    memcpy(&(request->response_buffer[request->response_buffer_size]), contents, realsize);
    request->response_buffer_size += realsize;
    request->response_buffer[request->response_buffer_size] = 0;
    
    return realsize;
}

size_t fetch_header_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    fetch_request_t *request = (fetch_request_t *)userp;
    size_t realsize = size * nmemb;
    
    char *ptr = realloc(request->header_buffer, request->header_buffer_size + realsize + 1);
    if (!ptr) {
        return 0; /* Out of memory */
    }
    
    request->header_buffer = ptr;
    memcpy(&(request->header_buffer[request->header_buffer_size]), contents, realsize);
    request->header_buffer_size += realsize;
    request->header_buffer[request->header_buffer_size] = 0;
    
    return realsize;
}

int fetch_progress_callback_internal(void *clientp, curl_off_t dltotal, curl_off_t dlnow, 
                                      curl_off_t ultotal, curl_off_t ulnow) {
    fetch_request_t *request = (fetch_request_t *)clientp;
    
    if (request->progress_callback) {
        request->progress_callback(request, (double)dltotal, (double)dlnow, 
                                 (double)ultotal, (double)ulnow, request->user_data);
    }
    
    return request->is_cancelled ? 1 : 0; /* Return 1 to abort */
}

void cleanup_free_json(void *jsonp) {
    cJSON **json = (cJSON **)jsonp; // expects cJSON**
    if (json && *json) {
        cJSON_Delete(*json);
        *json = NULL;
        LOG_DEBUG("AUTO", "JSON object freed");
    }
}

void cleanup_free_client(fetch_client_t **client) {
    if (client && *client) {
        // Libevent event_base freigeben
        if ((*client)->event_base) {
            event_base_free((*client)->event_base);
            (*client)->event_base = NULL;
        }
        // Libevent timer_event freigeben
        if ((*client)->timer_event) {
            event_free((*client)->timer_event);
            (*client)->timer_event = NULL;
        }
        // Default-Header freigeben
        if ((*client)->default_headers) {
            free((*client)->default_headers);
            (*client)->default_headers = NULL;
        }
        // CURLM Multi-Handle freigeben
        if ((*client)->multi_handle) {
            curl_multi_cleanup((*client)->multi_handle);
            (*client)->multi_handle = NULL;
        }
        free(*client);
        *client = NULL;
        LOG_DEBUG("AUTO", "fetch_client freed");
    }
}

void cleanup_free_request(fetch_request_t **request) {
    if (request && *request) {
        // CURL Handle freigeben
        if ((*request)->curl_handle) {
            curl_easy_cleanup((*request)->curl_handle);
            (*request)->curl_handle = NULL;
        }
        // Dynamische Strings und Buffer
        free((*request)->url);
        free((*request)->final_url);
        free((*request)->body);
        free((*request)->headers);
        free((*request)->form_fields);
        free((*request)->query_params);
        free((*request)->response_buffer);
        free((*request)->header_buffer);
        // Response-Struktur freigeben (eingebettete Felder leeren, struct selbst nicht free'n)
        fetch_response_t *resp = &(*request)->response;
        free(resp->body);
        resp->body = NULL;
        resp->body_size = 0;
        free(resp->headers);
        resp->headers = NULL;
        resp->headers_size = 0;
        if (resp->json) {
            fetch_json_destroy(resp->json);
            resp->json = NULL;
        }
        free(*request);
        *request = NULL;
        LOG_DEBUG("AUTO", "fetch_request freed");
    }
}

void cleanup_free_response(fetch_response_t **response) {
    if (response && *response) {
        free((*response)->body);
        free((*response)->headers);
        if ((*response)->json) {
            cleanup_free_json((void**)&((*response)->json));
        }
        free(*response);
        *response = NULL;
        LOG_DEBUG("AUTO", "fetch_response freed");
    }
}