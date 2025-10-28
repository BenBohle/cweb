#include <cweb/http.h>
#include <cweb/autofree.h>
#include <cweb/logger.h>
#include <cweb/leak_detector.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



void cweb_free_http_request(Request *req) {
    if (!req) return;
    for (int i = 0; i < req->header_count; i++) {
        if (req->headers[i].key) {
            cweb_leak_tracker_record("req.header.key", req->headers[i].key, strlen(req->headers[i].key) + 1, false);
            free(req->headers[i].key);
        }
        if (req->headers[i].value) {
            cweb_leak_tracker_record("req.header.value", req->headers[i].value, strlen(req->headers[i].value) + 1, false);
            free(req->headers[i].value);
        }
    }
    if (req->body) {
        cweb_leak_tracker_record("req.body", req->body, 0, false);
        free(req->body);
    }
    if (req->session_id) {
        cweb_leak_tracker_record("req.session_id", req->session_id, strlen(req->session_id) + 1, false);
        free(req->session_id);
    }
    cweb_leak_tracker_record("Request", req, sizeof(*req), false);
    free(req);
	LOG_DEBUG("HTTP", "Request freed");
}

static char* get_cookie_value(Request *req, const char* cookie_name) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, "Cookie") == 0) {
            char *cookie_header = strdup(req->headers[i].value);
            if (cookie_header) cweb_leak_tracker_record("cookie.header.copy", cookie_header, strlen(cookie_header) + 1, true);
            // TODO: check unused code
            // AUTOFREE char* cookie_header_to_free = cookie_header;

            char *cookie = strtok(cookie_header, "; ");
            while (cookie) {
                char *equals = strchr(cookie, '=');
                if (equals) {
                    *equals = '\0';
                    char *name = cookie;
                    char *value = equals + 1;
                    if (strcmp(name, cookie_name) == 0) {
                        LOG_DEBUG("HTTP", "Cookie found: %s=%s", name, value);
                        char *dup = strdup(value);
                        if (dup) cweb_leak_tracker_record("cookie.value", dup, strlen(dup) + 1, true);
                        cweb_leak_tracker_record("cookie.header.copy", cookie_header, strlen(cookie_header) + 1, false);
                        free(cookie_header);
                        return dup;
                    }
                }
                cookie = strtok(NULL, "; ");
            }
            LOG_DEBUG("HTTP", "Cookie not found: %s", cookie_name);
            cweb_leak_tracker_record("cookie.header.copy", cookie_header, strlen(cookie_header) + 1, false);
            free(cookie_header);
            return NULL;
        }
    }
	LOG_DEBUG("HTTP", "No Cookie header found");
    return NULL;
}

Request* cweb_parse_request(const char *raw_request, size_t len) {
    Request *req = calloc(1, sizeof(Request));
    if (!req) return NULL;
    cweb_leak_tracker_record("Request", req, sizeof(*req), true);

    char *buffer = strndup(raw_request, len);
    if (buffer) cweb_leak_tracker_record("request.buffer", buffer, len + 1, true);

	// TODO: check unused code
    // AUTOFREE char *buffer_to_free = buffer;

    char *saveptr;
    char *line = strtok_r(buffer, "\r\n", &saveptr);
    if (!line) { 

        if (buffer) { cweb_leak_tracker_record("request.buffer", buffer, len + 1, false);
            free(buffer); 
        }
        cweb_free_http_request(req); 
        return NULL; 
    }

    // Parse request line
    sscanf(line, "%15s %2047s %15s", req->method, req->path, req->version);

    // Parse headers
    while ((line = strtok_r(NULL, "\r\n", &saveptr))) {
        if (strlen(line) == 0) break; // End of headers

        char *colon = strchr(line, ':');
        if (colon && req->header_count < MAX_HEADERS) {
            *colon = '\0';
            char *key = line;
            char *value = colon + 1;
            while (isspace(*value)) value++; // Trim leading space

            req->headers[req->header_count].key = strdup(key);
            if (req->headers[req->header_count].key)
                cweb_leak_tracker_record("req.header.key", req->headers[req->header_count].key, strlen(req->headers[req->header_count].key) + 1, true);
            req->headers[req->header_count].value = strdup(value);
            if (req->headers[req->header_count].value)
                cweb_leak_tracker_record("req.header.value", req->headers[req->header_count].value, strlen(req->headers[req->header_count].value) + 1, true);
            req->header_count++;
        }
    }
    
    req->session_id = get_cookie_value(req, "session_id");
	LOG_DEBUG("HTTP", "Extracted session_id from cookie: %s", req->session_id ? req->session_id : "NULL");

	LOG_DEBUG("Parse request end", "Request method: %s, path: %s, version: %s", req->method, req->path, req->version);

    if (buffer) { cweb_leak_tracker_record("request.buffer", buffer, len + 1, false);
        free(buffer);
    }
    return req;
}

Response* cweb_create_response() {
    Response *res = calloc(1, sizeof(Response));
    if (res) {
        cweb_leak_tracker_record("Response", res, sizeof(*res), true);
        res->priority = 0; // Default priority
    }
    return res;
}

void cweb_free_http_response(Response *res) {
	LOG_DEBUG("HTTP", "Freeing response");
    if (!res) return;
    for (int i = 0; i < res->header_count; i++) {
        if (res->headers[i].key) {
            cweb_leak_tracker_record("res.header.key", res->headers[i].key, strlen(res->headers[i].key) + 1, false);
            free(res->headers[i].key);
        }
        if (res->headers[i].value) {
            cweb_leak_tracker_record("res.header.value", res->headers[i].value, strlen(res->headers[i].value) + 1, false);
            free(res->headers[i].value);
        }
    }
    if (res->body && res->isliteral != 1) {
        cweb_leak_tracker_record("res.body", res->body, res->body_len, false);
       
        free(res->body);
    }
    cweb_leak_tracker_record("Response", res, sizeof(*res), false);
    free(res);
}

void cweb_add_response_header(Response *res, const char *key, const char *value) {
	LOG_DEBUG("HTTP", "Adding response header: %s: %s", key, value);
    if (res->header_count < MAX_HEADERS) {
        res->headers[res->header_count].key = strdup(key);
        if (res->headers[res->header_count].key)
            cweb_leak_tracker_record("res.header.key", res->headers[res->header_count].key, strlen(res->headers[res->header_count].key) + 1, true);
        res->headers[res->header_count].value = strdup(value);
        if (res->headers[res->header_count].value)
            cweb_leak_tracker_record("res.header.value", res->headers[res->header_count].value, strlen(res->headers[res->header_count].value) + 1, true);
        res->header_count++;
    }
}

const char* cweb_get_response_header(const Response *res, const char *key) {
	if (!res || !key) {
		LOG_DEBUG("HTTP", "get_response_header invalid args");
		return NULL;
	}
	for (int i = 0; i < res->header_count; i++) {
		if (res->headers[i].key && strcasecmp(res->headers[i].key, key) == 0) {
			return res->headers[i].value; // Direktpointer, nicht freigeben
		}
	}
	return NULL;
}

void cweb_add_performance_headers(Response *res, const char *content_type) {
    // Add aggressive caching for static resources
    if (strstr(content_type, "image/") || 
        strstr(content_type, "text/css") || 
        strstr(content_type, "application/javascript") ||
        strstr(content_type, "font/")) {
        
        cweb_add_response_header(res, "Cache-Control", "public, max-age=31536000, immutable");
        cweb_add_response_header(res, "Expires", "Thu, 31 Dec 2025 23:59:59 GMT");
    } else if (strstr(content_type, "text/html")) {
        cweb_add_response_header(res, "Cache-Control", "public, max-age=300"); // 5 minutes for HTML
    }
    
    // Add compression hint
    cweb_add_response_header(res, "Vary", "Accept-Encoding");
    
    // security headers that don't hurt performance
    cweb_add_response_header(res, "X-Content-Type-Options", "nosniff");
    
}

const char* cweb_get_status_message(int code) {
    switch (code) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

char* cweb_serialize_response(Response *res, size_t *total_len) {
    AUTOFREE char *header_buf = NULL;
    size_t header_len = 0;
    
    // Initial status line
    char status_line[128];
    int status_len = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", res->status_code, cweb_get_status_message(res->status_code));
    header_len += status_len;

    // Content-Length is required
    char content_length_value[32];
    snprintf(content_length_value, sizeof(content_length_value), "%zu", res->body_len);
    cweb_add_response_header(res, "Content-Length", strdup(content_length_value));

    // Add other headers
    for (int i = 0; i < res->header_count; i++) {
        header_len += strlen(res->headers[i].key) + strlen(res->headers[i].value) + 4; // ": \r\n"
    }
    header_len += 2; // Final "\r\n"

    header_buf = malloc(header_len + 1);
    strcpy(header_buf, status_line);

    for (int i = 0; i < res->header_count; i++) {
        strcat(header_buf, res->headers[i].key);
        strcat(header_buf, ": ");
        strcat(header_buf, res->headers[i].value);
        strcat(header_buf, "\r\n");
    }
    strcat(header_buf, "\r\n");
    header_len = strlen(header_buf);

    *total_len = header_len + res->body_len;
    char *full_response = malloc(*total_len);
    memcpy(full_response, header_buf, header_len);
    if (res->body && res->body_len > 0) {
        memcpy(full_response + header_len, res->body, res->body_len);
    }

	LOG_DEBUG("HTTP", "Serialized response of total length: %zu", *total_len);
    return full_response;
}

const char* cweb_get_request_header(const Request *req, const char *key) {
    if (!req || !key) {
        LOG_DEBUG("HTTP", "get_request_header invalid args");
        return NULL;
    }
    for (int i = 0; i < req->header_count; i++) {
        if (req->headers[i].key && strcasecmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value; // Direktpointer, nicht freigeben
        }
    }
    return NULL;
}