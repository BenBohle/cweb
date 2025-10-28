#include "longtime.page.h"

static Response *g_response = NULL;

void longtime_page_assets() {
   
	cwagger_detail detail = {
		.request_schema = "",
		.response_schema = "{ \"type\": \"string\", \"enum\": [\"okay\"] }",
		.expected_arguments = ""
	};
	cwagger_add("GET", "/longtime", "Takes 15 min fetch time. represents non-blocking", &detail);

}
REGISTER_FRONTEND_ASSET(longtime_page_assets);

void longtime_apicall(fetch_request_t *request, fetch_response_t *response, void *user_data)
{
    if (fetch_response_get_status(response) == 200) {
        const fetch_json_t *json = fetch_response_get_json(response);
        if (json) {
            printf("full json: %s\n", fetch_json_to_string_pretty(json));
            g_response->body = fetch_json_to_string_pretty(json);
            g_response->body_len = strlen(g_response->body);
            cweb_add_response_header(g_response, "Content-Type", "application/json");
            g_response->status_code = 200;
            g_response->state = PROCESSED;
        }
    } else {
        LOG_DEBUG("FETCH_PAGE", "GitHub API request failed");
        g_response->status_code = 500;
        g_response->body = "Internal Server Error";
        g_response->body_len = strlen(g_response->body);
        cweb_add_response_header(g_response, "Content-Type", "text/plain");
        g_response->state = PROCESSED;
    }
}

void longtime_page(Request *req, Response *res) {

    if (strcmp(req->method, "POST") == 0) { 
        res->status_code = 403;
        res->body = "Method not allowed";
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
    }

    g_response = res;

    fetch(FETCH_GET, "https://awawaw.free.beeceptor.com/timeout", longtime_apicall, NULL, NULL);
}
