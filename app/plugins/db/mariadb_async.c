#include "mariadb_async.h"

#include <event2/event.h>
#include <event2/util.h>
#include <mariadb/mysql.h>
#include <cweb/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MARIADB_INVALID_SOCKET
#define MARIADB_INVALID_SOCKET -1
#endif

typedef enum {
    MDB_STATE_INIT = 0,
    MDB_STATE_CONNECTING,
    MDB_STATE_QUERYING,
    MDB_STATE_STORING_RESULT,
    MDB_STATE_FETCHING_ROWS,
    MDB_STATE_FINISHED,
    MDB_STATE_ERROR
} MariaDBInternalState;

typedef struct {
    struct event_base *base;
    MYSQL *mysql;
    MYSQL_RES *mysql_result;
    MariaDBResult result;

    struct event *event;
    MariaDBInternalState state;
    bool waiting_for_io;

    /* Configuration and query buffers */
    char *host;
    char *user;
    char *password;
    char *database;
    char *unix_socket;
    unsigned int port;
    unsigned long client_flag;
    unsigned int connect_timeout_ms;
    unsigned int read_timeout_ms;
    unsigned int write_timeout_ms;
    char *query;

    /* Status bookkeeping */
    MYSQL *connect_ret;
    int query_ret;
    MYSQL_ROW current_row;

    mariadb_async_query_cb callback;
    void *user_data;

    char error_message[256];
} MariaDBAsyncContext;

static void mariadb_async_context_free(MariaDBAsyncContext *ctx);
static void mariadb_async_finish(MariaDBAsyncContext *ctx, const char *error_message);
static void mariadb_async_drive(MariaDBAsyncContext *ctx, int io_status);
static void mariadb_async_reschedule(MariaDBAsyncContext *ctx, int wait_status);
static int mariadb_async_append_row(MariaDBAsyncContext *ctx, MYSQL_ROW row);

void mariadb_result_init(MariaDBResult *result) {
    if (!result) {
        return;
    }

    result->column_names = NULL;
    result->column_count = 0;
    result->rows = NULL;
    result->row_count = 0;
    result->affected_rows = 0;
}

static void mariadb_free_row(MariaDBRow *row, unsigned int column_count) {
    if (!row) {
        return;
    }

    if (row->values) {
        for (unsigned int i = 0; i < column_count; ++i) {
            free(row->values[i]);
        }
        free(row->values);
    }

    free(row->lengths);
}

void mariadb_result_free(MariaDBResult *result) {
    if (!result) {
        return;
    }

    if (result->column_names) {
        for (unsigned int i = 0; i < result->column_count; ++i) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }

    if (result->rows) {
        for (size_t i = 0; i < result->row_count; ++i) {
            mariadb_free_row(&result->rows[i], result->column_count);
        }
        free(result->rows);
    }

    mariadb_result_init(result);
}

int mariadb_result_clone(MariaDBResult *dest, const MariaDBResult *src) {
    if (!dest || !src) {
        return -1;
    }

    mariadb_result_init(dest);

    if (src->column_count > 0) {
        dest->column_names = calloc(src->column_count, sizeof(char *));
        if (!dest->column_names) {
            return -1;
        }
        dest->column_count = src->column_count;

        for (unsigned int i = 0; i < src->column_count; ++i) {
            if (src->column_names && src->column_names[i]) {
                dest->column_names[i] = strdup(src->column_names[i]);
                if (!dest->column_names[i]) {
                    mariadb_result_free(dest);
                    return -1;
                }
            }
        }
    }

    dest->row_count = src->row_count;
    if (src->row_count > 0 && src->column_count > 0) {
        dest->rows = calloc(src->row_count, sizeof(MariaDBRow));
        if (!dest->rows) {
            mariadb_result_free(dest);
            return -1;
        }

        for (size_t r = 0; r < src->row_count; ++r) {
            MariaDBRow *target = &dest->rows[r];
            const MariaDBRow *source = &src->rows[r];

            target->values = calloc(src->column_count, sizeof(char *));
            target->lengths = calloc(src->column_count, sizeof(unsigned long));
            if (!target->values || !target->lengths) {
                mariadb_result_free(dest);
                return -1;
            }

            for (unsigned int c = 0; c < src->column_count; ++c) {
                target->lengths[c] = source->lengths ? source->lengths[c] : 0;
                if (source->values && source->values[c]) {
                    target->values[c] = malloc(target->lengths[c] + 1);
                    if (!target->values[c]) {
                        mariadb_result_free(dest);
                        return -1;
                    }
                    memcpy(target->values[c], source->values[c], target->lengths[c]);
                    target->values[c][target->lengths[c]] = '\0';
                }
            }
        }
    }

    dest->affected_rows = src->affected_rows;
    return 0;
}

static void mariadb_async_set_error(MariaDBAsyncContext *ctx, const char *message) {
    if (!ctx) {
        return;
    }

    if (message) {
        strncpy(ctx->error_message, message, sizeof(ctx->error_message) - 1);
        ctx->error_message[sizeof(ctx->error_message) - 1] = '\0';
    } else if (ctx->mysql) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "%s", mysql_error(ctx->mysql));
    } else {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Unknown MariaDB error");
    }
}

static MariaDBAsyncContext *mariadb_async_context_new(struct event_base *base,
                                                      const MariaDBAsyncConfig *config,
                                                      const char *query,
                                                      mariadb_async_query_cb callback,
                                                      void *user_data) {
    MariaDBAsyncContext *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    ctx->base = base;
    ctx->callback = callback;
    ctx->user_data = user_data;
    mariadb_result_init(&ctx->result);
    ctx->state = MDB_STATE_INIT;

    if (config->host) ctx->host = strdup(config->host);
    if (config->user) ctx->user = strdup(config->user);
    if (config->password) ctx->password = strdup(config->password);
    if (config->database) ctx->database = strdup(config->database);
    if (config->unix_socket) ctx->unix_socket = strdup(config->unix_socket);

    ctx->port = config->port;
    ctx->client_flag = config->client_flag;
    ctx->connect_timeout_ms = config->connect_timeout_ms;
    ctx->read_timeout_ms = config->read_timeout_ms;
    ctx->write_timeout_ms = config->write_timeout_ms;

    if (query) {
        ctx->query = strdup(query);
    }

    if ((query && !ctx->query) ||
        (config->host && !ctx->host) ||
        (config->user && !ctx->user) ||
        (config->password && !ctx->password && config->password && *config->password) ||
        (config->database && !ctx->database) ||
        (config->unix_socket && !ctx->unix_socket)) {
        mariadb_async_set_error(ctx, "Failed to duplicate configuration string");
        mariadb_async_context_free(ctx);
        return NULL;
    }

    return ctx;
}

static void mariadb_async_context_free(MariaDBAsyncContext *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->event) {
        event_free(ctx->event);
        ctx->event = NULL;
    }

    if (ctx->mysql_result) {
        mysql_free_result(ctx->mysql_result);
        ctx->mysql_result = NULL;
    }

    if (ctx->mysql) {
        mysql_close(ctx->mysql);
        ctx->mysql = NULL;
    }

    free(ctx->host);
    free(ctx->user);
    free(ctx->password);
    free(ctx->database);
    free(ctx->unix_socket);
    free(ctx->query);

    mariadb_result_free(&ctx->result);
    free(ctx);
}

static void mariadb_async_finish(MariaDBAsyncContext *ctx, const char *error_message) {
    if (!ctx) {
        return;
    }

    MariaDBResult callback_result;
    MariaDBResult *result_ptr = NULL;

    if (!error_message) {
        callback_result = ctx->result;
        result_ptr = &callback_result;
        /* Transfer ownership to caller */
        mariadb_result_init(&ctx->result);
    }

    if (ctx->callback) {
        ctx->callback(result_ptr, error_message, ctx->user_data);
    }

    mariadb_async_context_free(ctx);
}

static void mariadb_async_event_cb(evutil_socket_t fd, short events, void *arg) {
    MariaDBAsyncContext *ctx = arg;
    int io_status = 0;

    if (events & EV_READ) {
        io_status |= MYSQL_WAIT_READ;
    }
    if (events & EV_WRITE) {
        io_status |= MYSQL_WAIT_WRITE;
    }
    if (events & EV_TIMEOUT) {
        io_status |= MYSQL_WAIT_TIMEOUT;
    }

    mariadb_async_drive(ctx, io_status);
}

static void mariadb_async_reschedule(MariaDBAsyncContext *ctx, int wait_status) {
    if (ctx->event) {
        event_free(ctx->event);
        ctx->event = NULL;
    }

    short ev_flags = 0;
    if (wait_status & MYSQL_WAIT_READ) {
        ev_flags |= EV_READ;
    }
    if (wait_status & MYSQL_WAIT_WRITE) {
        ev_flags |= EV_WRITE;
    }
    if (wait_status & MYSQL_WAIT_TIMEOUT) {
        ev_flags |= EV_TIMEOUT;
    }

    evutil_socket_t sock = -1;
    if (ev_flags & (EV_READ | EV_WRITE)) {
        sock = mysql_get_socket(ctx->mysql);
    }

    struct timeval tv = {0, 0};
    struct timeval *ptv = NULL;
    if (wait_status & MYSQL_WAIT_TIMEOUT) {
        unsigned int timeout_ms = mysql_get_timeout_value_ms(ctx->mysql);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ptv = &tv;
    }

    if (!(ev_flags & (EV_READ | EV_WRITE | EV_TIMEOUT))) {
        ev_flags = EV_READ | EV_WRITE;
        sock = mysql_get_socket(ctx->mysql);
    }

    ctx->event = event_new(ctx->base, sock, ev_flags, mariadb_async_event_cb, ctx);
    if (!ctx->event) {
        mariadb_async_set_error(ctx, "Failed to create libevent watcher");
        ctx->state = MDB_STATE_ERROR;
        mariadb_async_finish(ctx, ctx->error_message);
        return;
    }

    if (event_add(ctx->event, ptv) != 0) {
        mariadb_async_set_error(ctx, "Failed to add libevent watcher");
        ctx->state = MDB_STATE_ERROR;
        mariadb_async_finish(ctx, ctx->error_message);
    }
}

static int mariadb_async_prepare_connection(MariaDBAsyncContext *ctx) {
    ctx->mysql = mysql_init(NULL);
    if (!ctx->mysql) {
        mariadb_async_set_error(ctx, "mysql_init() failed");
        return -1;
    }

    unsigned int timeout = ctx->connect_timeout_ms / 1000;
    if (ctx->connect_timeout_ms && mysql_options(ctx->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) != 0) {
        mariadb_async_set_error(ctx, "Unable to set connect timeout");
        return -1;
    }

    timeout = ctx->read_timeout_ms / 1000;
    if (ctx->read_timeout_ms && mysql_options(ctx->mysql, MYSQL_OPT_READ_TIMEOUT, &timeout) != 0) {
        mariadb_async_set_error(ctx, "Unable to set read timeout");
        return -1;
    }

    timeout = ctx->write_timeout_ms / 1000;
    if (ctx->write_timeout_ms && mysql_options(ctx->mysql, MYSQL_OPT_WRITE_TIMEOUT, &timeout) != 0) {
        mariadb_async_set_error(ctx, "Unable to set write timeout");
        return -1;
    }

    
    if (mysql_optionsv(ctx->mysql, MYSQL_OPT_NONBLOCK, 0) != 0) {
        mariadb_async_set_error(ctx, "Unable to enable non-blocking mode");
        return -1;
    }

    return 0;
}

static int mariadb_async_step_connect(MariaDBAsyncContext *ctx, int io_status) {
    int status;

    if (!ctx->waiting_for_io) {
        ctx->waiting_for_io = true;
        status = mysql_real_connect_start(&ctx->connect_ret,
                                          ctx->mysql,
                                          ctx->host,
                                          ctx->user,
                                          ctx->password,
                                          ctx->database,
                                          ctx->port,
                                          ctx->unix_socket,
                                          ctx->client_flag);
    } else {
        status = mysql_real_connect_cont(&ctx->connect_ret, ctx->mysql, io_status);
    }

    if (status == 0) {
        ctx->waiting_for_io = false;
        if (!ctx->connect_ret) {
            mariadb_async_set_error(ctx, NULL);
            return -1;
        }
        ctx->state = MDB_STATE_QUERYING;
    }

    return status;
}

static int mariadb_async_step_query(MariaDBAsyncContext *ctx, int io_status) {
    int status;

    if (!ctx->waiting_for_io) {
        ctx->waiting_for_io = true;
        status = mysql_real_query_start(&ctx->query_ret,
                                        ctx->mysql,
                                        ctx->query,
                                        (unsigned long)strlen(ctx->query));
    } else {
        status = mysql_real_query_cont(&ctx->query_ret, ctx->mysql, io_status);
    }

    if (status == 0) {
        ctx->waiting_for_io = false;
        if (ctx->query_ret != 0) {
            mariadb_async_set_error(ctx, NULL);
            return -1;
        }

        unsigned int fields = mysql_field_count(ctx->mysql);
        if (fields == 0) {
            ctx->result.affected_rows = mysql_affected_rows(ctx->mysql);
            ctx->state = MDB_STATE_FINISHED;
        } else {
            ctx->state = MDB_STATE_STORING_RESULT;
        }
    }

    return status;
}

static int mariadb_async_step_store(MariaDBAsyncContext *ctx, int io_status) {
    int status;

    if (!ctx->waiting_for_io) {
        ctx->waiting_for_io = true;
        status = mysql_store_result_start(&ctx->mysql_result, ctx->mysql);
    } else {
        status = mysql_store_result_cont(&ctx->mysql_result, ctx->mysql, io_status);
    }

    if (status == 0) {
        ctx->waiting_for_io = false;

        if (!ctx->mysql_result) {
            if (mysql_errno(ctx->mysql)) {
                mariadb_async_set_error(ctx, NULL);
                return -1;
            }
            ctx->state = MDB_STATE_FINISHED;
            return 0;
        }

        unsigned int fields = mysql_num_fields(ctx->mysql_result);
        ctx->result.column_names = calloc(fields, sizeof(char *));
        if (!ctx->result.column_names) {
            mariadb_async_set_error(ctx, "Out of memory while allocating column names");
            return -1;
        }
        ctx->result.column_count = fields;

        MYSQL_FIELD *field_meta = mysql_fetch_fields(ctx->mysql_result);
        for (unsigned int i = 0; i < fields; ++i) {
            if (field_meta && field_meta[i].name) {
                ctx->result.column_names[i] = strdup(field_meta[i].name);
                if (!ctx->result.column_names[i]) {
                    mariadb_async_set_error(ctx, "Out of memory while copying column name");
                    return -1;
                }
            }
        }

        ctx->state = MDB_STATE_FETCHING_ROWS;
    }

    return status;
}

static int mariadb_async_step_fetch(MariaDBAsyncContext *ctx, int io_status) {
    int status;

    if (!ctx->waiting_for_io) {
        ctx->waiting_for_io = true;
        status = mysql_fetch_row_start(&ctx->current_row, ctx->mysql_result);
    } else {
        status = mysql_fetch_row_cont(&ctx->current_row, ctx->mysql_result, io_status);
    }

    if (status != 0) {
        return status;
    }

    ctx->waiting_for_io = false;

    if (!ctx->current_row) {
        ctx->state = MDB_STATE_FINISHED;
        return 0;
    }

    if (mariadb_async_append_row(ctx, ctx->current_row) != 0) {
        return -1;
    }

    /* Continue fetching immediately if data is already buffered. */
    return 0;
}

static int mariadb_async_append_row(MariaDBAsyncContext *ctx, MYSQL_ROW row) {
    size_t new_count = ctx->result.row_count + 1;
    MariaDBRow *rows = realloc(ctx->result.rows, new_count * sizeof(MariaDBRow));
    if (!rows) {
        mariadb_async_set_error(ctx, "Out of memory while appending row");
        return -1;
    }

    ctx->result.rows = rows;
    MariaDBRow *target = &ctx->result.rows[new_count - 1];
    memset(target, 0, sizeof(*target));

    unsigned int column_count = ctx->result.column_count;
    target->values = calloc(column_count, sizeof(char *));
    target->lengths = calloc(column_count, sizeof(unsigned long));
    if (!target->values || !target->lengths) {
        mariadb_async_set_error(ctx, "Out of memory while duplicating row");
        return -1;
    }

    unsigned long *lengths = mysql_fetch_lengths(ctx->mysql_result);
    for (unsigned int i = 0; i < column_count; ++i) {
        if (lengths) {
            target->lengths[i] = lengths[i];
        }
        if (row[i]) {
            target->values[i] = malloc(target->lengths[i] + 1);
            if (!target->values[i]) {
                mariadb_async_set_error(ctx, "Out of memory while copying field");
                return -1;
            }
            memcpy(target->values[i], row[i], target->lengths[i]);
            target->values[i][target->lengths[i]] = '\0';
        } else {
            target->values[i] = NULL;
        }
    }

    ctx->result.row_count = new_count;
    return 0;
}

static void mariadb_async_drive(MariaDBAsyncContext *ctx, int io_status) {
    if (!ctx) {
        return;
    }

    int status = 0;

    while (ctx->state != MDB_STATE_FINISHED && ctx->state != MDB_STATE_ERROR) {
        switch (ctx->state) {
            case MDB_STATE_INIT:
                if (mariadb_async_prepare_connection(ctx) != 0) {
                    ctx->state = MDB_STATE_ERROR;
                    break;
                }
                ctx->state = MDB_STATE_CONNECTING;
                ctx->waiting_for_io = false;
                io_status = 0;
                continue;
            case MDB_STATE_CONNECTING:
                status = mariadb_async_step_connect(ctx, io_status);
                break;
            case MDB_STATE_QUERYING:
                status = mariadb_async_step_query(ctx, io_status);
                break;
            case MDB_STATE_STORING_RESULT:
                status = mariadb_async_step_store(ctx, io_status);
                break;
            case MDB_STATE_FETCHING_ROWS:
                status = mariadb_async_step_fetch(ctx, io_status);
                if (status == 0) {
                    /* Prepare to immediately request the next row */
                    io_status = 0;
                    continue;
                }
                break;
            default:
                status = -1;
                break;
        }

        if (ctx->state == MDB_STATE_ERROR) {
            break;
        }

        if (status == 0) {
            io_status = 0;
            continue;
        } else if (status > 0) {
            mariadb_async_reschedule(ctx, status);
            return;
        } else {
            ctx->state = MDB_STATE_ERROR;
            break;
        }
    }

    if (ctx->state == MDB_STATE_FINISHED) {
        mariadb_async_finish(ctx, NULL);
    } else {
        if (ctx->error_message[0] == '\0') {
            mariadb_async_set_error(ctx, NULL);
        }
        mariadb_async_finish(ctx, ctx->error_message);
    }
}

int mariadb_async_query(struct event_base *base,
                        const MariaDBAsyncConfig *config,
                        const char *query,
                        mariadb_async_query_cb callback,
                        void *user_data) {
    if (!base || !config || !query || !callback) {
        return -1;
    }

    MariaDBAsyncContext *ctx = mariadb_async_context_new(base, config, query, callback, user_data);
    if (!ctx) {
        return -1;
    }

    mariadb_async_drive(ctx, 0);
    return 0;
}


void cleanup_free_mariadb_result(MariaDBResult *result) {
    if (result) {
        mariadb_result_free(result);
        LOG_DEBUG("AUTO", "MariaDBResult freed");
    }
}