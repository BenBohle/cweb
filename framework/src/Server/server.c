#include <cweb/server.h>
#include <cweb/fileserver.h>
#include <cweb/compress.h>
#include <cweb/speedbench.h>
#include "../../app/includes/cstyles.h"
#include <cweb/leak_detector.h>

// Forward declarations (Prototypen) für die Callbacks
static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                        struct sockaddr *addr, int socklen, void *ctx);
static void conn_read_cb(struct bufferevent *bev, void *ctx);
static void conn_event_cb(struct bufferevent *bev, short events, void *ctx);

struct event_base *g_event_base = NULL;


struct event_base *cweb_get_event_base() {
    return g_event_base;
}

void cweb_run_server(const char *port) {
    // Create the main event loop
    g_event_base = event_base_new();
    if (!g_event_base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        exit(1);
    }
    printf("Event base created.\n");
    cweb_init_pending_responses(g_event_base);
	cweb_init_speedbench();

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(atoi(port));

    // Create a listener to accept new connections
    struct evconnlistener *listener = evconnlistener_new_bind(
        g_event_base, listener_cb, NULL,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
        (struct sockaddr*)&sin, sizeof(sin));

    if (!listener) {
        perror("Couldn't create listener");
        exit(1);
    }
    
    LOG_INFO("SERVER", "port %s", port);
    printf("\n\n "fg_green vconnection RESET " RUNNING on " fg_cyan underline "http://localhost:%s\n\n" RESET, port);

    // Start the event loop. This function only returns on error.
    int ret = event_base_dispatch(g_event_base);
    if (ret == -1) {
        fprintf(stderr, "Error running event loop\n");
    } else if (ret == 1) {
        fprintf(stderr, "No events were registered\n");
    }

    // Cleanup
    cweb_cleanup_pending_responses();
    LOG_INFO("SERVER", "End of server execution, cleaning up resources");
    evconnlistener_free(listener);
    event_base_free(g_event_base);
    g_event_base = NULL;
}

// This callback is executed when a new TCP connection is accepted.
static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                        struct sockaddr *addr, int socklen, void *ctx) {

    // TODO HJEREEE!!
    (void )ctx;
    (void)socklen; 

    struct sockaddr_in *sin = (struct sockaddr_in*)addr;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
    LOG_INFO("SERVER", "New connection from %s:%d", ip, ntohs(sin->sin_port));

    struct event_base *base = evconnlistener_get_base(listener);

    // Create a buffered event to manage the connection
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent for new connection\n");
        close(fd); // Schließe den Socket, um Ressourcen freizugeben
        return;
    }
    cweb_leak_tracker_record("bufferevent", bev, 0, true);

    // Set the callbacks for read and event events (like disconnect)
    bufferevent_setcb(bev, conn_read_cb, NULL, conn_event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

// This callback is executed when there is data to read on a connection.
static void conn_read_cb(struct bufferevent *bev, void *ctx) {
// todo überarbeitne
    (void)ctx; // Unbenutzter Parameter

    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    LOG_DEBUG("SERVER", "Received %zu bytes of data", len);
    if (len == 0) return;

    // Read the data from the buffer into a C string
    AUTOFREE char *data = malloc(len + 1);
    if (!data) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    cweb_leak_tracker_record("server.data", data, len + 1, true);
    evbuffer_remove(input, data, len);
    data[len] = '\0';

    Request *req = cweb_parse_request(data, len);
    if (!req) {
        fprintf(stderr, "Failed to parse request\n");
        return; // Kein Speicher wurde bisher allokiert, daher kein `free` nötig
    }
    if (!req) return; // Bad request

	cweb_speedbench_start(req, req->path);

    const char* old_session_id = req->session_id;
    if (req->using_session) {
        LOG_DEBUG("SESSION", "Session associated with request: %s", req->session ? req->session->id : "NULL");
        req->session = get_or_create_session(old_session_id);

        // Aktualisiere req->session_id mit der neuen Session-ID
        if (req->session) {
            if (req->session_id) {
                free(req->session_id); // Alte session_id freigeben
            }
            req->session_id = strdup(req->session->id);
            LOG_DEBUG("SESSION", "Updated session_id: %s", req->session_id);
        }
        LOG_DEBUG("SESSION", "2 Session associated with request: %s", req->session ? req->session->id : "NULL");
    }
    
    Response *res = cweb_create_response();
    if (!res) {
        fprintf(stderr, "Failed to create response\n");
        cweb_free_http_request(req);
        return;
    }
    res->status_code = 404; // Default to 404

    // If a new session was created, set the cookie in the response
    if (req->session && (!old_session_id || strcmp(old_session_id, req->session->id) != 0)) {
        AUTOFREE char* cookie_val = NULL;
        if (asprintf(&cookie_val, "session_id=%s; HttpOnly; Path=/; Max-Age=%d", req->session->id, SESSION_LIFETIME) < 0) {
            LOG_ERROR("SET_COOKIE", "Failed to allocate session cookie for %s", req->session->id);
        } else {
            cweb_add_response_header(res, "Set-Cookie", cookie_val);
            LOG_DEBUG("SET_COOKIE", "Set-Cookie header added: %s", cookie_val);
        }
    }

    route_handler_t handler = cweb_get_route_handler(req->path, &req->using_session);
    LOG_DEBUG("REQ session bool", "Using session for request: %s", req->using_session ? "true" : "false");

    if (handler) {
        LOG_DEBUG("ROUTING", "Found handler for path: %s", req->path);
        handler(req, res);
    } else if (cweb_fileserver_is_static_file(req->path) == true) {
        // Try to serve as static file
        LOG_DEBUG("SERVER", "Serving static file: %s", req->path);
        cweb_fileserver_handle_request(req, res);
    } else {
        res->body = strdup("<h1>404 Not Found</h1>");
        res->body_len = strlen(res->body);
        res->status_code = 404;
        cweb_add_response_header(res, "Content-Type", "text/html");
        res->state = PROCESSED;
        cweb_leak_tracker_record(" res->body ", res->body, res->body_len, true);
        LOG_DEBUG("SERVER", "No handler found for path: %s, returning 404", req->path);
    }

    if (res->state == PROCESSED) {
        cweb_send_response(bev, req, res);
    } else {
        cweb_add_pending_response(req, res, bev);
        LOG_DEBUG("SERVER", "Response pending, added to async queue");
    }
}

static int path_is_compressible(const char *path) {
    if (!path) return 0;
    const char *dot = strrchr(path, '.');
    if (!dot || !*(dot+1)) return 0;
    const char *ext = dot + 1;

    // Komprimierbare Typen
    const char *ok[] = {"html","htm","css","js","mjs","json","txt","xml","svg", NULL};
    for (int i = 0; ok[i]; ++i) if (strcasecmp(ext, ok[i]) == 0) return 1;

    // Nicht komprimieren (binär/bereits komprimiert)
    const char *no[] = {"png","jpg","jpeg","gif","webp","avif","bmp","ico",
                        "mp4","webm","mp3","wav","pdf","zip","gz","br",
                        "woff","woff2","ttf","otf","eot", NULL};
    for (int i = 0; no[i]; ++i) if (strcasecmp(ext, no[i]) == 0) return 0;

    return 0;
}

void cweb_send_response(struct bufferevent *bev, Request *req, Response *res) {
    if (!bev || !req || !res) {
        LOG_ERROR("SEND_RESPONSE", "Invalid args (bev=%p req=%p res=%p)", (void*)bev, (void*)req, (void*)res);
        return;
    }
    if (res->state != PROCESSED) {
        LOG_DEBUG("SEND_RESPONSE", "Response not ready");
        return;
    }


	/* testing compression options for eacha lone */
	// char *out = NULL;
	// size_t out_len = 0;
	
	// cweb_brotli(res->body, res->body_len, &out, &out_len);
	// cweb_gzip(res->body, res->body_len, &out, &out_len);
	// free(res->body);
	// res->body = out;
	// res->body_len = out_len;
	// cweb_add_response_header(res, "Content-Encoding", "gzip");
	// cweb_add_response_header(res, "Vary", "Accept-Encoding");
	
	int do_compress = path_is_compressible(req->path) && res->body && res->body_len > 512;

    // App-Benchmark hier beenden (ohne Kompression/Serialisierung)


    // Optional: Kompression (auto_compress sollte Accept-Encoding prüfen)
    if (do_compress) {
        struct timespec c0, c1; clock_gettime(CLOCK_MONOTONIC, &c0);
        LOG_DEBUG("SEND_RESPONSE", "auto_compress %s (%zu bytes)", req->path, res->body_len);
        cweb_auto_compress(req, res); // intern: gzip lvl 3–5 oder brotli q≈4–5
        clock_gettime(CLOCK_MONOTONIC, &c1);
        double cms = (c1.tv_sec - c0.tv_sec)*1000.0 + (c1.tv_nsec - c0.tv_nsec)/1e6;
        LOG_WARNING("SEND_RESPONSE", "compress_dur=%.2fms after=%zu bytes", cms, res->body_len);
    } else {
        LOG_WARNING("SEND_RESPONSE", "skip compress %s (%zu bytes)", req->path, res->body_len);
    }

    size_t response_len = 0;
    AUTOFREE char *response_str = cweb_serialize_response(res, &response_len);
    if (!response_str) {
        LOG_ERROR("SEND_RESPONSE", "serialize_response failed");
        cweb_free_http_response(res);
        cweb_free_http_request(req);
        return;
    }

	cweb_speedbench_end(req);
   
    if (bufferevent_write(bev, response_str, response_len) != 0) {
        LOG_ERROR("SEND_RESPONSE", "bufferevent_write failed");
    }

    LOG_DEBUG("SERVER", "Sent response %d (%zu bytes body)", res->status_code, res->body_len);

    cweb_free_http_response(res);
    cweb_free_http_request(req);
    cweb_leak_tracker_dump();
}

// This callback is executed when a connection is closed or an error occurs.
static void conn_event_cb(struct bufferevent *bev, short events, void *ctx) {

    (void)ctx; // Unbenutzter Parameter

    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // The bufferevent will be freed, and the underlying socket closed,
        // because we used the BEV_OPT_CLOSE_ON_FREE option.
        LOG_INFO("SERVER", "Connection closed or error occurred");
        cweb_cancel_pending_responses(bev);
        cweb_leak_tracker_record("bufferevent", bev, 0, false);
        bufferevent_free(bev);
    }
}

int configure_fileserver(FileServerMode mode, size_t max_file_size, bool auto_reload) {
    FileServerConfig fs_config = {0};
    fs_config.static_dir = "./assets";                  // Current directory (contains img/, tufte.css, example.html)
    fs_config.cache_file = "./build/static_cache.bin";  // Binary cache file
	fs_config.lookup_path = "/assets/";				//  path normalization
    fs_config.mode = mode;      // Try memory first, fallback to filesystem
    fs_config.auto_reload = auto_reload;                 // Auto-reload changed files in development
    fs_config.max_file_size = max_file_size;  // 10MB max file size for caching

    if (cweb_fileserver_init(&fs_config) != 0) {
        LOG_ERROR("SERVER", "Failed to initialize file server");
        return 1;
    }

    LOG_INFO("SERVER", "File server ready. All images will be served from ultra-fast cache! <3");

    return 0;
}

void cweb_cleanup_server() {
    LOG_INFO("SERVER", "CWeb server shutting down...");
    cweb_cleanup_pending_responses();
    cweb_clear_routes();
    cweb_fileserver_destroy();
    session_store_destroy();
}


void cweb_cleanup_free_event(struct event **evp) {
    if (evp && *evp) {
        event_free(*evp);
        *evp = NULL;
        LOG_DEBUG("AUTO", "event freed");
    }
}