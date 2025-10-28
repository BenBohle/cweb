#ifndef CWEB_HTTP_H
#define CWEB_HTTP_H

#include <cweb/leak_detector.h>
#include <stddef.h>
#include <stdbool.h>
#include <cweb/session.h>


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HEADERS 64
#define MAX_PATH_LEN 2048
#define READ_BUFFER_SIZE 8192

typedef struct {
    char *key;
    char *value;
} Header;

typedef enum {
    NPROCESSED = 0,  // Noch nicht verarbeitet (Initialzustand)
    PROCESSING,       // Wird gerade verarbeitet (z.B. während async fetch)
    PROCESSED,        // Vollständig verarbeitet und bereit zum Senden
    ERROR             // Fehler aufgetreten (z.B. bei fetch-Fehler)
} ResponseState;

typedef struct {
    char method[16];
    char path[MAX_PATH_LEN];
    char version[16];
    Header headers[MAX_HEADERS];
    int header_count;
    char *body;
    char *session_id; // Extracted from cookie
    Session *session; // Associated session object
	bool using_session;
} Request;

typedef struct {
    int status_code;
    const char *status_message;
    Header headers[MAX_HEADERS];
    int header_count;
    char *body;
    size_t body_len;
        int priority;
        int isliteral; // 0 = dynamic, 1 = literal/static
    ResponseState state;
    void *async_data;
    void (*async_cancel)(void *async_data);
} Response;

// Request lifecycle
Request* cweb_parse_request(const char *raw_request, size_t len);
void cweb_free_http_request(Request *req);

// Response lifecycle
Response* cweb_create_response();
void cweb_free_http_response(Response *res);
char* cweb_serialize_response(Response *res, size_t *total_len);
void cweb_add_response_header(Response *res, const char *key, const char *value);
void cweb_add_performance_headers(Response *res, const char *content_type);
void cweb_add_preload_headers(Response *res);
const char* cweb_get_status_message(int code);
const char* cweb_get_request_header(const Request *req, const char *key);
const char* cweb_get_response_header(const Response *res, const char *key);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_HTTP_H */
