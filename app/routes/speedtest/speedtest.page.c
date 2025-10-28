#include "speedtest.page.h"
#include <cweb/speedbench.h>


void speedtest_page_assets() {
   
	cwagger_detail detail = {
		.request_schema = "",
		.response_schema = "{ \"type\": \"object\", \"properties\": { \"path\": { \"type\": \"string\" }, \"start_wall\": { \"type\": \"string\" }, \"end_wall\": { \"type\": \"string\" }, \"duration_ms\": { \"type\": \"number\" } }, \"required\": [\"path\", \"start_wall\", \"end_wall\", \"duration_ms\"] }",
		.expected_arguments = ""
	};
	cwagger_add("GET", "/speedtest", "Overview of the time to send speed. Speedtest.", &detail);

}
REGISTER_FRONTEND_ASSET(speedtest_page_assets);

void speedtest_page(Request *req, Response *res) {
    if (strcmp(req->method, "POST") == 0) { 
        res->status_code = 403;
        res->body = "Method not allowed";
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain; charset=utf-8");
        return;
    }

    LOG_INFO("SPEEDTEST", "speed benchmark: %s", req->path);

    size_t out_count = 0;
    const SpeedSample *samples = cweb_get_speed_history(&out_count); // erwartet size_t*
    
    // Body aufbauen (ohne Leaks)
    res->body = NULL;
    for (size_t i = 0; i < out_count; i++) {
        const char *p = samples[i].path;
        char *new_body = NULL;
        if (asprintf(&new_body, "%s/* Sample %zu: path=%s duration=%.2fms */\n",
                     res->body ? res->body : "", i, p, samples[i].duration_ms) < 0) {
            LOG_ERROR("SPEEDTEST", "asprintf failed while building response");
            free(res->body);
            res->body = strdup("Internal Server Error");
            res->status_code = 500;
            res->body_len = strlen(res->body);
            cweb_add_response_header(res, "Content-Type", "text/plain; charset=utf-8");
            res->state = PROCESSED;
            return;
        }
        free(res->body);
        res->body = new_body;
    }

    if (!res->body) {
        res->body = strdup("/* No samples available */\n");
    }

    res->body_len = strlen(res->body);
    res->status_code = 200;
    cweb_add_response_header(res, "Content-Type", "text/plain; charset=utf-8");
    res->state = PROCESSED;
}