#include "easyfetch.page.h"
// #include <cweb/server.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Callback context passed to fetch callbacks so they also receive the Response
 * pointer in addition to the fetch data. This keeps the fetch API unchanged
 * while giving the callback access to the request/response context. */
typedef struct github_cb_ctx {
    fetch_data_t *data;
    Response *res;
    bool cancelled;
} github_cb_ctx_t;

static void github_ctx_cancel(void *user_data) {
    github_cb_ctx_t *ctx = (github_cb_ctx_t *)user_data;
    if (!ctx) {
        return;
    }
    ctx->cancelled = true;
}

void easyfetch_page_assets() {
   
	cwagger_detail detail = {
		.request_schema = "",
		.response_schema = "github api data from octocat user",
		.expected_arguments = ""
	};
	cwagger_add("GET", "/easyfetch", "use of standard fetch function witout customs", &detail);

}
REGISTER_FRONTEND_ASSET(easyfetch_page_assets);

static const char *safe_string(const char *value) {
    return value ? value : "(null)";
}

void github_apicall(fetch_request_t *request, fetch_response_t *response, void *user_data)
{
    github_cb_ctx_t *ctx = (github_cb_ctx_t *)user_data;
    if (!ctx) {
        LOG_DEBUG("FETCH_PAGE", "github_apicall called with NULL context");
        cweb_speedbench_end(request);
        return;
    }
    fetch_data_t *data = ctx->data;
    Response *res = ctx->res;
    bool success = false;

    if (ctx->cancelled) {
        LOG_DEBUG("FETCH_PAGE", "Client disconnected before GitHub fetch completed");
        if (res) {
            res->status_code = 499;
            res->body = NULL;
            res->body_len = 0;
            res->isliteral = 0;
            res->state = PROCESSED;
        }
        goto cleanup;
    }

    if (fetch_response_get_status(response) == 200) {
        const fetch_json_t *json = fetch_response_get_json(response);
        if (json) {
            const char *login = fetch_json_object_get_string(json, "login");
            const char *avatar_url = fetch_json_object_get_string(json, "avatar_url");
            const char *html_url = fetch_json_object_get_string(json, "html_url");
            const char *type = fetch_json_object_get_string(json, "type");

            if (login) data->login = strdup(login);
            if (avatar_url) data->avatar_url = strdup(avatar_url);
            if (html_url) data->html_url = strdup(html_url);
            if (type) data->type = strdup(type);

            data->id = (int)fetch_json_object_get_number(json, "id");
            data->public_repos = (int)fetch_json_object_get_number(json, "public_repos");
            data->followers = (int)fetch_json_object_get_number(json, "followers");
            data->following = (int)fetch_json_object_get_number(json, "following");
            data->generated_time = time(NULL);

            char *html = fetch_template(data);
            if (html) {
                res->body = html;
                res->body_len = strlen(res->body);
                cweb_add_response_header(res, "Content-Type", "text/html");
                res->status_code = 200;
                res->state = PROCESSED;
                success = true;
            } else {
                LOG_ERROR("FETCH_PAGE", "Failed to render fetch template");
            }
        } else {
            LOG_ERROR("FETCH_PAGE", "GitHub API response missing JSON payload");
        }
    } else {
        LOG_DEBUG("FETCH_PAGE", "GitHub API request failed");
    }

    if (!success) {
        res->status_code = 500;
        res->body = "Internal Server Error. Couldnt get data from GitHub API";
        res->isliteral = 1;
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
    }

cleanup:
    if (res) {
        res->async_data = NULL;
        res->async_cancel = NULL;
    }
    cweb_speedbench_end(request);

    printf("Login: %s\n", safe_string(data->login));
    printf("ID: %d\n", data->id);
    printf("Avatar URL: %s\n", safe_string(data->avatar_url));
    printf("HTML URL: %s\n", safe_string(data->html_url));
    printf("Type: %s\n", safe_string(data->type));
    printf("Public Repos: %d\n", data->public_repos);
    printf("Followers: %d\n", data->followers);
    printf("Following: %d\n", data->following);

    free((char *)data->login);
    free(data->avatar_url);
    free(data->html_url);
    free(data->type);
    free(data);

    /* free the small callback context wrapper */
    free(ctx);
}

void easyfetch_page(Request *req, Response *res) {

    fetch_data_t *data = malloc(sizeof(fetch_data_t));
    memset(data, 0, sizeof(fetch_data_t));

    if (strcmp(req->method, "POST") == 0) { 
        res->status_code = 403;
        res->body = "Method not allowed";
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
    }

    /* create small context to hand both the fetch data and Response* to the
     * callback (keeps fetch API unchanged). The callback frees this wrapper. */
    github_cb_ctx_t *ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        LOG_ERROR("FETCH_PAGE", "failed to allocate callback context");
        res->status_code = 500;
        res->body = "Internal Server Error";
        res->body_len = strlen(res->body);
        res->isliteral = 1;
        res->state = PROCESSED;
        return;
    }
    ctx->data = data;
    ctx->res = res;
    ctx->cancelled = false;

    res->async_data = ctx;
    res->async_cancel = github_ctx_cancel;

    fetch(FETCH_GET, "https://api.github.com/users/octocat", github_apicall, NULL, ctx);
}
