#ifndef CWEB_CWAGGER_H
#define CWEB_CWAGGER_H

#include <stddef.h>
#include <cweb/http.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CWAGGER_MAX_ENDPOINTS 128
#define CWAGGER_MAX_METHOD_LEN 16
#define CWAGGER_MAX_PATH_LEN 256
#define CWAGGER_MAX_DESCRIPTION_LEN 256
#define CWAGGER_MAX_SCHEMA_LEN 512

typedef struct {
    char request_schema[CWAGGER_MAX_SCHEMA_LEN];
    char response_schema[CWAGGER_MAX_SCHEMA_LEN];
    char expected_arguments[CWAGGER_MAX_SCHEMA_LEN];
} cwagger_detail;

typedef struct {
    char method[CWAGGER_MAX_METHOD_LEN];
    char path[CWAGGER_MAX_PATH_LEN];
    char short_description[CWAGGER_MAX_DESCRIPTION_LEN];
    cwagger_detail detail;
} cwagger_endpoint;

typedef struct {
    char doc_path[CWAGGER_MAX_PATH_LEN];
    cwagger_endpoint endpoints[CWAGGER_MAX_ENDPOINTS];
    size_t endpoint_count;
} cwagger_doc;

void cwagger_init(const char *doc_path);
int cwagger_add(const char *method, const char *path,
                const char *short_description, const cwagger_detail *detail);

const cwagger_doc *cwagger_get_doc(void);
cwagger_endpoint *cwagger_get_endpoints(void);
const cwagger_endpoint *cwagger_get_endpoint(size_t index);
size_t cwagger_get_endpoint_count(void);
const char *cwagger_get_doc_path(void);

void cweb_cwaggerdoc_page(Request *req, Response *res);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_CWAGGER_H */