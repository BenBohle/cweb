#include <cweb/fetch.h>
#include <cweb/server.h>
#include <cweb/speedbench.h>
#include <cweb/logger.h>
#include <stdlib.h>
#include <string.h>
#include "cweb_fetch_internal.h"

void fetch(fetch_method_t method, const char *url, fetch_callback_t callback, const fetch_json_t *json, void *data) {

    // überarbeiten TODO!
    (void)json;

    LOG_INFO("CWEB/FETCH", "Starting fetch for URL: %s", url);
    if (fetch_global_init() != FETCH_OK) {
        LOG_ERROR("CWEB/FETCH", "Failed to initialize fetch library");
        return;
    }
    
    if (!g_event_base) {
        LOG_ERROR("CWEB/FETCH", "Invalid event_base provided");
        fetch_global_cleanup();
        return;
    }
    
    fetch_config_t config = fetch_config_default();
    fetch_client_t *client = fetch_client_create(g_event_base, &config);
    if (!client) {
        LOG_ERROR("CWEB/FETCH", "Failed to create client");
        fetch_global_cleanup();
        return;
    }

    fetch_request_t *request = fetch_request_create(client, method, url);
    if (!request) {
        LOG_ERROR("CWEB/FETCH", "Failed to create request");
        fetch_client_destroy(client);
        fetch_global_cleanup();
        return;
    }
	cweb_speedbench_start(request, url);

    fetch_request_set_callback(request, callback, data);
    
    if (fetch_request_execute(request) != FETCH_OK) {
        LOG_ERROR("CWEB/FETCH", "Failed to execute fetch");
        fetch_request_destroy(request);
        fetch_client_destroy(client);
        fetch_global_cleanup();
        return;
    }

    LOG_DEBUG("CWEB/FETCH", "Fetch request executed, added to event loop");
    
    LOG_DEBUG("CWEB/FETCH", "fetch request submitted to event loop (non-blocking)");
}

