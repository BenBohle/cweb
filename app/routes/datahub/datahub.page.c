#include "datahub.page.h"

#include "../../plugins/async_state_machine.h"
#include <cweb/server.h>

#include <event2/buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// cwagger get routes hinzufugen sodass er alle autoamtisch anzeigt aber nur die entsprechenden gesetztne mit den emta daten
void datahub_page_assets() {
   
	cwagger_detail detail = {
		.request_schema = "",
		.response_schema = "",
		.expected_arguments = ""
	};
	cwagger_add("GET", "/datahub", "maraidb aufruf und fetch", &detail);

}
REGISTER_FRONTEND_ASSET(datahub_page_assets);

typedef struct {
    Response *response;
    AsyncStateMachine machine;
    char *github_username;
} DatahubContext;

static void datahub_context_free(DatahubContext *ctx) {
    if (!ctx) {
        return;
    }

    async_state_machine_cleanup(&ctx->machine);
    free(ctx->github_username);
    free(ctx);
}

static void append_html_escaped(struct evbuffer *buf, const char *text) {
    if (!text) {
        evbuffer_add_printf(buf, "<em>null</em>");
        return;
    }

    for (const char *p = text; *p; ++p) {
        switch (*p) {
            case '&': evbuffer_add_printf(buf, "&amp;"); break;
            case '<': evbuffer_add_printf(buf, "&lt;"); break;
            case '>': evbuffer_add_printf(buf, "&gt;"); break;
            case '"': evbuffer_add_printf(buf, "&quot;"); break;
            case '\'': evbuffer_add_printf(buf, "&#39;"); break;
            default: evbuffer_add_printf(buf, "%c", *p); break;
        }
    }
}

static void render_github_section(struct evbuffer *buf, const AsyncAggregatedData *data) {
    evbuffer_add_printf(buf, "<section><h2>GitHub Profile</h2>");
    if (data->has_github) {
        evbuffer_add_printf(buf, "<p><strong>Login:</strong> ");
        append_html_escaped(buf, data->github.login);
        evbuffer_add_printf(buf, "</p><p><strong>Public repos:</strong> %d</p>", data->github.public_repos);
        evbuffer_add_printf(buf, "<p><strong>Followers:</strong> %d, <strong>Following:</strong> %d</p>",
                            data->github.followers, data->github.following);
        if (data->github.avatar_url) {
            evbuffer_add_printf(buf, "<p><img src=\"");
            append_html_escaped(buf, data->github.avatar_url);
            evbuffer_add_printf(buf, "\" alt=\"avatar\" width=120 height=120></p>");
        }
        if (data->github.html_url) {
            evbuffer_add_printf(buf, "<p><a href=\"");
            append_html_escaped(buf, data->github.html_url);
            evbuffer_add_printf(buf, "\" target=\"_blank\">View profile</a></p>");
        }
    } else {
        evbuffer_add_printf(buf, "<p class=\"error\">%s</p>",
                            data->fetch_error ? data->fetch_error_message : "Fetch in progress");
    }
    evbuffer_add_printf(buf, "</section>");
}

static void render_db_section(struct evbuffer *buf, const AsyncAggregatedData *data) {
    evbuffer_add_printf(buf, "<section><h2>MariaDB Query</h2>");
    if (data->has_db_result) {
        if (data->db_result.column_count == 0) {
            evbuffer_add_printf(buf, "<p>Query executed successfully (no result set). Affected rows: %llu</p>",
                                data->db_result.affected_rows);
        } else {
            evbuffer_add_printf(buf, "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\"><thead><tr>");
            for (unsigned int c = 0; c < data->db_result.column_count; ++c) {
                evbuffer_add_printf(buf, "<th>");
                append_html_escaped(buf, data->db_result.column_names[c]);
                evbuffer_add_printf(buf, "</th>");
            }
            evbuffer_add_printf(buf, "</tr></thead><tbody>");
            for (size_t r = 0; r < data->db_result.row_count; ++r) {
                evbuffer_add_printf(buf, "<tr>");
                for (unsigned int c = 0; c < data->db_result.column_count; ++c) {
                    evbuffer_add_printf(buf, "<td>");
                    append_html_escaped(buf, data->db_result.rows[r].values[c]);
                    evbuffer_add_printf(buf, "</td>");
                }
                evbuffer_add_printf(buf, "</tr>");
            }
            if (data->db_result.row_count == 0) {
                evbuffer_add_printf(buf, "<tr><td colspan=\"%u\"><em>No rows returned.</em></td></tr>",
                                    data->db_result.column_count);
            }
            evbuffer_add_printf(buf, "</tbody></table>");
        }
    } else {
        evbuffer_add_printf(buf, "<p class=\"error\">%s</p>",
                            data->db_error ? data->db_error_message : "Waiting for database response");
    }
    evbuffer_add_printf(buf, "</section>");
}

static void datahub_completion_cb(const AsyncAggregatedData *data, void *user_data) {
    DatahubContext *ctx = user_data;
    if (!ctx || !data) {
        return;
    }

    Response *res = ctx->response;
    struct evbuffer *buf = evbuffer_new();
    if (!buf) {
        res->status_code = 500;
        res->body = strdup("<h1>Internal Server Error</h1><p>Unable to allocate buffer.</p>");
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/html; charset=utf-8");
        res->state = PROCESSED;
        datahub_context_free(ctx);
        return;
    }

    evbuffer_add_printf(buf, "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>Data Hub</title>"
                             "<style>body{font-family:Arial,Helvetica,sans-serif;margin:2rem;}"
                             "section{margin-bottom:2rem;}"
                             "table{border-collapse:collapse;width:100%%;}"
                             "th,td{text-align:left;}"
                             ".error{color:#b00;}</style></head><body>");
    evbuffer_add_printf(buf, "<h1>Async Data Hub</h1>");

    render_github_section(buf, data);
    render_db_section(buf, data);

    evbuffer_add_printf(buf, "<footer><p>Generated at %ld</p></footer></body></html>", (long)time(NULL));

    size_t len = evbuffer_get_length(buf);
    char *body = malloc(len + 1);
    if (!body) {
        evbuffer_free(buf);
        res->status_code = 500;
        res->body = strdup("<h1>Internal Server Error</h1><p>Unable to allocate body.</p>");
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/html; charset=utf-8");
        res->state = PROCESSED;
        datahub_context_free(ctx);
        return;
    }

    evbuffer_remove(buf, body, len);
    body[len] = '\0';
    evbuffer_free(buf);

    res->status_code = (data->fetch_error && data->db_error) ? 502 : 200;
    res->body = body;
    res->body_len = len;
    cweb_add_response_header(res, "Content-Type", "text/html; charset=utf-8");
    res->state = PROCESSED;

    datahub_context_free(ctx);
}

static const char *get_env_or_default(const char *name, const char *fallback) {
    const char *value = getenv(name);
    return (value && *value) ? value : fallback;
}

void datahub_page(Request *req, Response *res) {
    if (strcmp(req->method, "GET") != 0) {
        res->status_code = 405;
        res->body = "Method not allowed";
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
    }

    DatahubContext *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        res->status_code = 500;
        res->body = "Internal server error";
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
    }

    ctx->response = res;
    async_state_machine_init(&ctx->machine, g_event_base, datahub_completion_cb, ctx);

    const char *username = get_env_or_default("GITHUB_USERNAME", "BenBohle");
    ctx->github_username = strdup(username);
    if (!ctx->github_username) {
        res->status_code = 500;
        res->body = "Internal server error";
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        datahub_context_free(ctx);
        return;
    }

    GithubFetchConfig fetch_cfg = {
        .username = ctx->github_username,
        .user_agent = NULL
    };

    async_state_machine_start_github_fetch(&ctx->machine, &fetch_cfg);

    const char *db_host = getenv("MYSQL_HOST");
    const char *db_user = getenv("MYSQL_USER");
    const char *db_pass = getenv("MYSQL_PASSWORD");
    const char *db_name = getenv("MYSQL_DATABASE");
    const char *db_port_env = getenv("MYSQL_PORT");

    if (db_host && db_user && db_name) {
        unsigned long port = db_port_env ? strtoul(db_port_env, NULL, 10) : 3306UL;
        MariaDBAsyncConfig db_cfg = {
            .host = db_host,
            .port = (unsigned int)port,
            .user = db_user,
            .password = db_pass,
            .database = db_name,
            .unix_socket = NULL,
            .client_flag = 0,
            .connect_timeout_ms = 5000,
            .read_timeout_ms = 5000,
            .write_timeout_ms = 5000
        };

        const char *query = "SELECT NOW() AS server_time, USER() AS user_name, DATABASE() AS current_schema";
        async_state_machine_start_db_query(&ctx->machine, &db_cfg, query);
    } else {
        ctx->machine.data.db_error = true;
        strncpy(ctx->machine.data.db_error_message,
                "MariaDB environment variables not configured (MARIADB_HOST, MARIADB_USER, MARIADB_DATABASE)",
                sizeof(ctx->machine.data.db_error_message) - 1);
    }

    if (ctx->machine.pending_count == 0) {
        datahub_completion_cb(&ctx->machine.data, ctx);
        return;
    }

    res->status_code = 200;
    res->isliteral = 1;
    res->body = "Processing";
    res->body_len = strlen(res->body);
    cweb_add_response_header(res, "Content-Type", "text/plain");
    res->state = PROCESSING;
}
