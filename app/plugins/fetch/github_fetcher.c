#include "github_fetcher.h"

#include <cweb/fetch.h>

#include <event2/event.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_USER_AGENT "CWeb-Framework/1.0"

typedef struct {
    struct event_base *base;
    github_fetch_callback callback;
    void *user_data;
    fetch_client_t *client;
    fetch_request_t *request;
    char *url;
    char *user_agent;
    fetch_data_t data;
} GithubFetchContext;

static int ensure_fetch_initialised(void) {
    static int initialised = 0;
    if (!initialised) {
        if (fetch_global_init() != FETCH_OK) {
            return -1;
        }
        initialised = 1;
    }
    return 0;
}

static void github_fetch_context_free(GithubFetchContext *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->request) {
        fetch_request_destroy(ctx->request);
        ctx->request = NULL;
    }

    if (ctx->client) {
        fetch_client_destroy(ctx->client);
        ctx->client = NULL;
    }

    github_fetch_data_cleanup(&ctx->data);
    free(ctx->url);
    free(ctx->user_agent);
    free(ctx);
}

static void github_fetch_callback_internal(fetch_request_t *request,
                                           fetch_response_t *response,
                                           void *user_data) {
    GithubFetchContext *ctx = user_data;
    const char *error_message = NULL;

    if (!response || fetch_response_get_status(response) != 200) {
        error_message = "GitHub API request failed";
    } else {
        const fetch_json_t *json = fetch_response_get_json(response);
        if (!json) {
            error_message = "GitHub API did not return JSON";
        } else {
            const char *login = fetch_json_object_get_string(json, "login");
            const char *avatar_url = fetch_json_object_get_string(json, "avatar_url");
            const char *html_url = fetch_json_object_get_string(json, "html_url");
            const char *type = fetch_json_object_get_string(json, "type");

            if (login) ctx->data.login = strdup(login);
            if (avatar_url) ctx->data.avatar_url = strdup(avatar_url);
            if (html_url) ctx->data.html_url = strdup(html_url);
            if (type) ctx->data.type = strdup(type);

            ctx->data.id = (int)fetch_json_object_get_number(json, "id");
            ctx->data.public_repos = (int)fetch_json_object_get_number(json, "public_repos");
            ctx->data.followers = (int)fetch_json_object_get_number(json, "followers");
            ctx->data.following = (int)fetch_json_object_get_number(json, "following");
            ctx->data.generated_time = time(NULL);
        }
    }

    if (ctx->callback) {
        ctx->callback(error_message ? NULL : &ctx->data, error_message, ctx->user_data);
    }

    github_fetch_context_free(ctx);
}

int github_fetch_user(struct event_base *base,
                      const GithubFetchConfig *config,
                      github_fetch_callback callback,
                      void *user_data) {
    if (!base || !config || !config->username || !callback) {
        return -1;
    }

    if (ensure_fetch_initialised() != 0) {
        return -1;
    }

    GithubFetchContext *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        return -1;
    }

    ctx->base = base;
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->data.generated_time = time(NULL);

    const char *user_agent = config->user_agent ? config->user_agent : DEFAULT_USER_AGENT;
    ctx->user_agent = strdup(user_agent);
    if (!ctx->user_agent) {
        github_fetch_context_free(ctx);
        return -1;
    }

    size_t url_len = strlen(config->username) + strlen("https://api.github.com/users/") + 1;
    ctx->url = malloc(url_len);
    if (!ctx->url) {
        github_fetch_context_free(ctx);
        return -1;
    }
    snprintf(ctx->url, url_len, "https://api.github.com/users/%s", config->username);

    fetch_config_t fetch_config = fetch_config_default();
    fetch_config.user_agent = ctx->user_agent;
    ctx->client = fetch_client_create(base, &fetch_config);
    if (!ctx->client) {
        github_fetch_context_free(ctx);
        return -1;
    }

    ctx->request = fetch_request_create(ctx->client, FETCH_GET, ctx->url);
    if (!ctx->request) {
        github_fetch_context_free(ctx);
        return -1;
    }

    fetch_request_set_header(ctx->request, "Accept", "application/vnd.github.v3+json");
    fetch_request_set_header(ctx->request, "User-Agent", ctx->user_agent);

    fetch_request_set_callback(ctx->request, github_fetch_callback_internal, ctx);

    if (fetch_request_execute(ctx->request) != FETCH_OK) {
        github_fetch_context_free(ctx);
        return -1;
    }

    return 0;
}

int github_fetch_data_copy(fetch_data_t *dest, const fetch_data_t *src) {
    if (!dest || !src) {
        return -1;
    }

    memset(dest, 0, sizeof(*dest));
    dest->generated_time = src->generated_time;
    dest->id = src->id;
    dest->public_repos = src->public_repos;
    dest->followers = src->followers;
    dest->following = src->following;

    if (src->login) {
        dest->login = strdup(src->login);
        if (!dest->login) goto fail;
    }
    if (src->avatar_url) {
        dest->avatar_url = strdup(src->avatar_url);
        if (!dest->avatar_url) goto fail;
    }
    if (src->html_url) {
        dest->html_url = strdup(src->html_url);
        if (!dest->html_url) goto fail;
    }
    if (src->type) {
        dest->type = strdup(src->type);
        if (!dest->type) goto fail;
    }

    return 0;

fail:
    github_fetch_data_cleanup(dest);
    return -1;
}

void github_fetch_data_cleanup(fetch_data_t *data) {
    if (!data) {
        return;
    }

    free((char *)data->login);
    free(data->avatar_url);
    free(data->html_url);
    free(data->type);
    memset(data, 0, sizeof(*data));
}
