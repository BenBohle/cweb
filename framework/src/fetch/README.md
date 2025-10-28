# HttpAPI - High-Performance C HTTP Client Library

A modern, high-performance HTTP client library for C that integrates seamlessly with libevent for non-blocking operations. Designed for web frameworks and high-performance applications.

## Features

- **Non-blocking I/O**: Full integration with libevent for async operations
- **JSON Support**: Built-in JSON parsing and generation using cJSON
- **Developer Friendly**: Clean, intuitive API with comprehensive error handling
- **High Performance**: Optimized for speed and low memory usage
- **Modular Design**: Well-structured codebase with clear separation of concerns
- **Security First**: SSL/TLS support with certificate verification
- **Thread-Safe**: Designed for single-threaded event-driven applications

## Key Benefits

- **Zero Thread Overhead**: Uses event-driven architecture instead of threads
- **Memory Efficient**: Careful memory management with no leaks
- **Production Ready**: Comprehensive error handling and edge case coverage
- **Easy Integration**: Simple API that works with existing libevent applications
- **Extensible**: Modular design allows for easy customization

## Dependencies

- **libcurl** (>= 7.50.0) - HTTP client functionality
- **libevent** (>= 2.1.0) - Event loop integration
- **cJSON** (>= 1.7.0) - JSON parsing and generation

## Installation

### From Source

\`\`\`bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libcurl4-openssl-dev libevent-dev libcjson-dev

# Install dependencies (CentOS/RHEL)
sudo yum install libcurl-devel libevent-devel cjson-devel

# Build and install
make
sudo make install
\`\`\`

### Using pkg-config

After installation, you can use pkg-config to get compiler flags:

\`\`\`bash
gcc myapp.c $(pkg-config --cflags --libs fetch)
\`\`\`

## Quick Start

### Simple GET Request

\`\`\`c
#include <event2/event.h>
#include "fetch.h"

void response_callback(fetch_request_t *request, fetch_response_t *response, void *user_data) {
    printf("Status: %ld\n", fetch_response_get_status(response));
    printf("Body: %s\n", fetch_response_get_body(response));
    
    // Stop event loop
    event_base_loopbreak((struct event_base *)user_data);
    fetch_request_destroy(request);
}

int main() {
    // Initialize
    fetch_global_init();
    struct event_base *base = event_base_new();
    
    // Create client
    fetch_config_t config = fetch_config_default();
    fetch_client_t *client = fetch_client_create(base, &config);
    
    // Create and execute request
    fetch_request_t *request = fetch_request_create(client, FETCH_GET, "https://api.example.com/data");
    fetch_request_set_callback(request, response_callback, base);
    fetch_request_execute(request);
    
    // Run event loop
    event_base_dispatch(base);
    
    // Cleanup
    fetch_client_destroy(client);
    event_base_free(base);
    fetch_global_cleanup();
    return 0;
}
\`\`\`

### JSON POST Request

\`\`\`c
// Create JSON payload
fetch_json_t *json = fetch_json_create_object();
fetch_json_object_set_string(json, "name", "John Doe");
fetch_json_object_set_number(json, "age", 30);

// Create request with JSON body
fetch_request_t *request = fetch_request_create(client, FETCH_POST, "https://api.example.com/users");
fetch_request_set_json_body(request, json);
fetch_request_set_callback(request, response_callback, base);
fetch_request_execute(request);

fetch_json_destroy(json);
\`\`\`

## API Reference

### Core Functions

#### Library Initialization
\`\`\`c
fetch_error_t fetch_global_init(void);
void fetch_global_cleanup(void);
\`\`\`

#### Client Management
\`\`\`c
fetch_client_t *fetch_client_create(struct event_base *base, const fetch_config_t *config);
void fetch_client_destroy(fetch_client_t *client);
\`\`\`

#### Request Creation and Execution
\`\`\`c
fetch_request_t *fetch_request_create(fetch_client_t *client, fetch_method_t method, const char *url);
fetch_error_t fetch_request_execute(fetch_request_t *request);
void fetch_request_destroy(fetch_request_t *request);
\`\`\`

### Configuration

\`\`\`c
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

fetch_config_t fetch_config_default(void);
\`\`\`

### JSON API

\`\`\`c
// Creation
fetch_json_t *fetch_json_create_object(void);
fetch_json_t *fetch_json_create_array(void);
fetch_json_t *fetch_json_parse(const char *json_string);

// Object operations
fetch_error_t fetch_json_object_set_string(fetch_json_t *json, const char *key, const char *value);
fetch_error_t fetch_json_object_set_number(fetch_json_t *json, const char *key, double value);
fetch_error_t fetch_json_object_set_bool(fetch_json_t *json, const char *key, bool value);

const char *fetch_json_object_get_string(const fetch_json_t *json, const char *key);
double fetch_json_object_get_number(const fetch_json_t *json, const char *key);
bool fetch_json_object_get_bool(const fetch_json_t *json, const char *key);
\`\`\`

## Advanced Usage

### Custom Headers

\`\`\`c
// Set default headers for all requests
fetch_client_set_default_header(client, "Authorization", "Bearer token123");

// Set request-specific headers
fetch_request_set_header(request, "Content-Type", "application/json");
fetch_request_set_header(request, "X-Custom-Header", "value");
\`\`\`

### Form Data

\`\`\`c
// Add form fields
fetch_request_add_form_field(request, "username", "john");
fetch_request_add_form_field(request, "password", "secret");

// Add file upload
fetch_request_add_form_file(request, "avatar", "/path/to/image.jpg", "image/jpeg");
\`\`\`

### Query Parameters

\`\`\`c
fetch_request_add_query_param(request, "page", "1");
fetch_request_add_query_param(request, "limit", "10");
fetch_request_add_query_param(request, "sort", "name");
\`\`\`

### Progress Tracking

\`\`\`c
void progress_callback(fetch_request_t *request, double dl_total, double dl_now, 
                      double ul_total, double ul_now, void *user_data) {
    if (dl_total > 0) {
        double progress = (dl_now / dl_total) * 100.0;
        printf("Download progress: %.1f%%\n", progress);
    }
}

fetch_request_set_progress_callback(request, progress_callback, NULL);
\`\`\`

## Integration with Web Frameworks

This library is designed to integrate seamlessly with C web frameworks:

\`\`\`c
// In your web framework's request handler
void api_handler(struct evhttp_request *req, void *arg) {
    // Create HTTP API request
    fetch_request_t *api_req = fetch_request_create(client, FETCH_GET, "https://api.service.com/data");
    
    // Store web request context for callback
    fetch_request_set_callback(api_req, api_response_handler, req);
    fetch_request_execute(api_req);
    
    // Don't send response yet - will be sent in callback
}

void api_response_handler(fetch_request_t *api_req, fetch_response_t *api_resp, void *user_data) {
    struct evhttp_request *web_req = (struct evhttp_request *)user_data;
    
    // Forward API response to web client
    evhttp_send_reply(web_req, fetch_response_get_status(api_resp), 
                     "OK", evbuffer_new_from_string(fetch_response_get_body(api_resp)));
    
    fetch_request_destroy(api_req);
}
\`\`\`

## Error Handling

\`\`\`c
typedef enum {
    FETCH_OK = 0,
    FETCH_ERROR_MEMORY = -1,
    FETCH_ERROR_INVALID_PARAM = -2,
    FETCH_ERROR_CURL = -3,
    FETCH_ERROR_JSON = -4,
    FETCH_ERROR_TIMEOUT = -5,
    FETCH_ERROR_NETWORK = -6
} fetch_error_t;

const char *fetch_error_string(fetch_error_t error);
\`\`\`

## Performance Tips

1. **Reuse Clients**: Create one client per event base and reuse it
2. **Connection Pooling**: libcurl automatically handles connection reuse
3. **Memory Management**: Always destroy requests after use
4. **Batch Operations**: Use the same event loop for multiple requests
5. **Compression**: Enable compression for better bandwidth usage

## Security Best Practices

1. **SSL Verification**: Always verify SSL certificates in production
2. **Timeouts**: Set appropriate timeouts to prevent hanging requests
3. **Input Validation**: Validate all URLs and data before making requests
4. **Memory Safety**: The library handles memory management safely
5. **Error Handling**: Always check return values and handle errors

## Building Examples

\`\`\`bash
make examples
./examples/simple_get
./examples/json_post
\`\`\`

## Running Tests

\`\`\`bash
make test
\`\`\`

## Memory Leak Detection

\`\`\`bash
make memcheck
\`\`\`

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Run `make format` to format code
6. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Changelog

### v1.0.0
- Initial release
- Full libevent integration
- JSON support
- Comprehensive API
- Production-ready stability
