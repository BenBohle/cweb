#include <cweb/server.h>
#include <event2/event.h>
#include <cweb/leak_detector.h>
#include <stdbool.h>

// Structure to track pending responses
typedef struct pending_response {
    Request *req;
    Response *res;
    struct bufferevent *bev;
    struct pending_response *next;
    bool cancelled;
} pending_response_t;

static pending_response_t *pending_responses = NULL;
static struct event *check_timer = NULL;

// Add a pending response to the list
void cweb_add_pending_response(Request *req, Response *res, struct bufferevent *bev) {
    pending_response_t *pending = malloc(sizeof(pending_response_t));
    pending->req = req;
    pending->res = res;
    pending->bev = bev;
    pending->next = pending_responses;
    pending->cancelled = false;
    pending_responses = pending;
    cweb_leak_tracker_record("pending_struct", pending, sizeof(pending_response_t), true);
}

void cweb_cancel_pending_responses(struct bufferevent *bev) {
    pending_response_t *current = pending_responses;
    while (current) {
        if (current->bev == bev) {
            current->bev = NULL;
            current->cancelled = true;
            if (current->res && current->res->async_cancel) {
                current->res->async_cancel(current->res->async_data);
                current->res->async_data = NULL;
                current->res->async_cancel = NULL;
            }
        }
        current = current->next;
    }
}

// Timer callback to check for processed responses
static void check_pending_responses(evutil_socket_t fd, short events, void *arg) {
    (void)fd;     // Suppress unused parameter warning
    (void)events; // Suppress unused parameter warning  
    (void)arg;    // Suppress unused parameter warning
    
    pending_response_t **current = &pending_responses;
    
    while (*current) {
        pending_response_t *pending = *current;
        if (!pending || !pending->res) {
            // Defensive: unlink invalid node
            *current = pending ? pending->next : NULL;
            if (pending) {
                cweb_leak_tracker_record("pending_struct", pending, sizeof(pending_response_t), false);
                free(pending);
            }
            continue;
        }
        
        if (pending->res->state == PROCESSED) {
            LOG_DEBUG("SERVER_PENDING", "Processing completed response for request");
            *current = pending->next;

            if (!pending->cancelled && pending->bev) {
                cweb_send_response(pending->bev, pending->req, pending->res);
            } else {
                cweb_free_http_response(pending->res);
                cweb_free_http_request(pending->req);
            }

            cweb_leak_tracker_record("pending_struct", pending, sizeof(pending_response_t), false);
            free(pending);
        } else {
            /* advance to next pointer (no removal) */
            current = &pending->next;
        }
    }
}

// Initialize the pending response system
void cweb_init_pending_responses(struct event_base *base) {
    struct timeval tv = {0, 100000}; // Check every 100ms
    check_timer = event_new(base, -1, EV_PERSIST, check_pending_responses, NULL);
    cweb_leak_tracker_record("pending_checktimer", check_timer, 10, true);
    event_add(check_timer, &tv);
}

// Cleanup pending responses
void cweb_cleanup_pending_responses() {
    if (check_timer) {
        cweb_leak_tracker_record("pending_checktimer", check_timer, 10, false);
        event_free(check_timer);
        check_timer = NULL;
    }
    
    while (pending_responses) {
        pending_response_t *next = pending_responses->next;
        cweb_leak_tracker_record("pending_responses->req", pending_responses->req, sizeof(pending_response_t), false);
        cweb_free_http_request(pending_responses->req);
        cweb_leak_tracker_record("pending_responses->req", pending_responses->res, sizeof(pending_response_t), false);
        cweb_free_http_response(pending_responses->res);
        cweb_leak_tracker_record("pending_responses", pending_responses, sizeof(pending_responses), false);
        free(pending_responses);
        pending_responses = next;
    }
}
