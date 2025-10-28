#include <cweb/fetch.h>
#include "cweb_fetch_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Response API */
long fetch_response_get_status(const fetch_response_t *response) {
    return response ? response->status_code : 0;
}

const char *fetch_response_get_body(const fetch_response_t *response) {
    return response ? response->body : NULL;
}

size_t fetch_response_get_body_size(const fetch_response_t *response) {
    return response ? response->body_size : 0;
}

const char *fetch_response_get_headers(const fetch_response_t *response) {
    return response ? response->headers : NULL;
}

const char *fetch_response_get_header(const fetch_response_t *response,
                                        const char *name) {
    if (!response || !response->headers || !name) {
        return NULL;
    }
    
    char *headers = response->headers;
    char *line = strtok(headers, "\r\n");
    
    while (line) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *header_name = line;
            char *header_value = colon + 1;
            
            /* Skip whitespace */
            while (*header_value == ' ' || *header_value == '\t') {
                header_value++;
            }
            
            if (strcasecmp(header_name, name) == 0) {
                return header_value;
            }
            
            *colon = ':'; /* Restore the colon */
        }
        line = strtok(NULL, "\r\n");
    }
    
    return NULL;
}

const fetch_json_t *fetch_response_get_json(const fetch_response_t *response) {
    if (!response) {
        return NULL;
    }
    
    /* Lazy parse JSON if not already done */
    if (!response->json && response->body) {
        fetch_response_t *mutable_response = (fetch_response_t *)response;
        mutable_response->json = fetch_json_parse(response->body);
    }
    
    return response->json;
}

double fetch_response_get_total_time(const fetch_response_t *response) {
    return response ? response->total_time : 0.0;
}
