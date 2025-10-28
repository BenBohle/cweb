#ifndef CWEB_FETCH_H
#define CWEB_FETCH_H

#include <event2/event.h>
#include <curl/curl.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  */
#define AUTOFREE_FETCH_REQUEST __attribute__((cleanup(cleanup_free_request)))
#define AUTOFREE_FETCH_RESPONSE __attribute__((cleanup(cleanup_free_response)))
#define AUTOFREE_CJSON __attribute__((cleanup(cleanup_free_json)))

/* Forward declarations */
typedef struct fetch_client fetch_client_t;
typedef struct fetch_request fetch_request_t;
typedef struct fetch_response fetch_response_t;
typedef struct fetch_json fetch_json_t;

/* HTTP methods */
typedef enum {
    FETCH_GET = 0,
    FETCH_POST,
    FETCH_PUT,
    FETCH_DELETE,
    FETCH_PATCH,
    FETCH_HEAD,
    FETCH_OPTIONS
} fetch_method_t;

/* Error codes */
typedef enum {
    FETCH_OK = 0,
    FETCH_ERROR_MEMORY = -1,
    FETCH_ERROR_INVALID_PARAM = -2,
    FETCH_ERROR_CURL = -3,
    FETCH_ERROR_JSON = -4,
    FETCH_ERROR_TIMEOUT = -5,
    FETCH_ERROR_NETWORK = -6
} fetch_error_t;

/* Response structure */
struct fetch_response {
    long status_code;
    char *body;
    size_t body_size;
    char *headers;
    size_t headers_size;
    fetch_json_t *json;
    double total_time;
    fetch_error_t error;
};

/* Request completion callback */
typedef void (*fetch_callback_t)(fetch_request_t *request, 
                                 fetch_response_t *response, 
                                 void *user_data);

/* Progress callback */
typedef void (*fetch_progress_callback_t)(fetch_request_t *request,
                                          double download_total,
                                          double download_now,
                                          double upload_total,
                                          double upload_now,
                                          void *user_data);

/* Client configuration */
typedef struct {
    const char *user_agent;
    long timeout_ms;
    long connect_timeout_ms;
    bool follow_redirects;
    long max_redirects;
    bool verify_ssl;
    const char *ca_cert_path;
    const char *proxy_url;
    bool enable_compression;
} fetch_config_t;

/* === Core API === */

void fetch(fetch_method_t method, const char *url, fetch_callback_t callback, const fetch_json_t *json, void *data);

/* Initialize the library (call once at startup) */
fetch_error_t fetch_global_init(void);

/* Cleanup the library (call once at shutdown) */
void fetch_global_cleanup(void);

/* Create a new HTTP client */
fetch_client_t *fetch_client_create(struct event_base *base, 
                                    const fetch_config_t *config);

/* Destroy HTTP client */
void fetch_client_destroy(fetch_client_t *client);

/* Set default headers for all requests */
fetch_error_t fetch_client_set_default_header(fetch_client_t *client,
                                              const char *name,
                                              const char *value);

/* Remove default header */
fetch_error_t fetch_client_remove_default_header(fetch_client_t *client,
                                                 const char *name);

/* === Request API === */

/* Create a new request */
fetch_request_t *fetch_request_create(fetch_client_t *client,
                                      fetch_method_t method,
                                      const char *url);

/* Destroy request */
void fetch_request_destroy(fetch_request_t *request);

/* Set request header */
fetch_error_t fetch_request_set_header(fetch_request_t *request,
                                       const char *name,
                                       const char *value);

/* Set request body (raw data) */
fetch_error_t fetch_request_set_body(fetch_request_t *request,
                                     const void *data,
                                     size_t size);

/* Set request body from JSON */
fetch_error_t fetch_request_set_json_body(fetch_request_t *request,
                                          const fetch_json_t *json);

/* Set form data */
fetch_error_t fetch_request_add_form_field(fetch_request_t *request,
                                           const char *name,
                                           const char *value);

/* Set file upload */
fetch_error_t fetch_request_add_form_file(fetch_request_t *request,
                                          const char *name,
                                          const char *filename,
                                          const char *content_type);

/* Set query parameter */
fetch_error_t fetch_request_add_query_param(fetch_request_t *request,
                                            const char *name,
                                            const char *value);

/* Set callbacks */
fetch_error_t fetch_request_set_callback(fetch_request_t *request,
                                         fetch_callback_t callback,
                                         void *user_data);

fetch_error_t fetch_request_set_progress_callback(fetch_request_t *request,
                                                  fetch_progress_callback_t callback,
                                                  void *user_data);

/* Execute request asynchronously */
fetch_error_t fetch_request_execute(fetch_request_t *request);

/* Cancel request */
fetch_error_t fetch_request_cancel(fetch_request_t *request);

/* === Response API === */

/* Get response status code */
long fetch_response_get_status(const fetch_response_t *response);

/* Get response body */
const char *fetch_response_get_body(const fetch_response_t *response);
size_t fetch_response_get_body_size(const fetch_response_t *response);

/* Get response headers */
const char *fetch_response_get_headers(const fetch_response_t *response);
const char *fetch_response_get_header(const fetch_response_t *response,
                                      const char *name);

/* Get response as JSON */
const fetch_json_t *fetch_response_get_json(const fetch_response_t *response);

/* Get timing information */
double fetch_response_get_total_time(const fetch_response_t *response);

/* === JSON API === */

/* Create JSON object */
fetch_json_t *fetch_json_create_object(void);
fetch_json_t *fetch_json_create_array(void);

/* Parse JSON from string */
fetch_json_t *fetch_json_parse(const char *json_string);

/* Convert JSON to string */
char *fetch_json_to_string(const fetch_json_t *json);
char *fetch_json_to_string_pretty(const fetch_json_t *json);

/* Destroy JSON object */
void fetch_json_destroy(fetch_json_t *json);

/* Object operations */
fetch_error_t fetch_json_object_set_string(fetch_json_t *json,
                                           const char *key,
                                           const char *value);
fetch_error_t fetch_json_object_set_number(fetch_json_t *json,
                                           const char *key,
                                           double value);
fetch_error_t fetch_json_object_set_bool(fetch_json_t *json,
                                         const char *key,
                                         bool value);
fetch_error_t fetch_json_object_set_null(fetch_json_t *json,
                                         const char *key);
fetch_error_t fetch_json_object_set_object(fetch_json_t *json,
                                           const char *key,
                                           fetch_json_t *value);

/* Get operations */
const char *fetch_json_object_get_string(const fetch_json_t *json,
                                         const char *key);
double fetch_json_object_get_number(const fetch_json_t *json,
                                    const char *key);
bool fetch_json_object_get_bool(const fetch_json_t *json,
                                const char *key);
fetch_json_t *fetch_json_object_get_object(const fetch_json_t *json,
                                           const char *key);

/* Array operations */
fetch_error_t fetch_json_array_add_string(fetch_json_t *json,
                                          const char *value);
fetch_error_t fetch_json_array_add_number(fetch_json_t *json,
                                          double value);
fetch_error_t fetch_json_array_add_bool(fetch_json_t *json,
                                        bool value);
fetch_error_t fetch_json_array_add_object(fetch_json_t *json,
                                          fetch_json_t *value);

size_t fetch_json_array_size(const fetch_json_t *json);
fetch_json_t *fetch_json_array_get(const fetch_json_t *json, size_t index);

/* === Utility Functions === */

/* URL encoding */
char *fetch_url_encode(const char *str);
char *fetch_url_decode(const char *str);

/* Get error string */
const char *fetch_error_string(fetch_error_t error);

/* Get default configuration */
fetch_config_t fetch_config_default(void);

/* === Cleanup functions for AUTOFREE === */
void cleanup_free_json(void *jsonp);
void cleanup_free_client(fetch_client_t **client);
void cleanup_free_request(fetch_request_t **request);
void cleanup_free_response(fetch_response_t **response);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_FETCH_H */
