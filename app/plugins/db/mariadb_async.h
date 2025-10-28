#ifndef APP_PLUGINS_DB_MARIADB_ASYNC_H
#define APP_PLUGINS_DB_MARIADB_ASYNC_H

#include <event2/event.h>
#include <stddef.h>

#define AUTOFREE_MARIADB_RESULT __attribute__((cleanup(cleanup_free_mariadb_result)))

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Developer friendly asynchronous MariaDB connector that integrates with the
 * global libevent base. The connector exposes a small, callback driven API
 * which mirrors the behaviour of the fetch plugin so both data sources can be
 * orchestrated through the same state machine.
 */

typedef struct {
    char **values;            /* Array of column values (NULL for SQL NULL). */
    unsigned long *lengths;   /* Byte lengths for each column value.        */
} MariaDBRow;

typedef struct {
    char **column_names;      /* Array of column names. */
    unsigned int column_count;
    MariaDBRow *rows;         /* Dynamic array of result rows. */
    size_t row_count;
    unsigned long long affected_rows; /* For statements without result sets. */
} MariaDBResult;

typedef struct {
    const char *host;
    unsigned int port;
    const char *user;
    const char *password;
    const char *database;
    const char *unix_socket;
    unsigned long client_flag;
    unsigned int connect_timeout_ms;
    unsigned int read_timeout_ms;
    unsigned int write_timeout_ms;
} MariaDBAsyncConfig;

typedef void (*mariadb_async_query_cb)(const MariaDBResult *result,
                                       const char *error_message,
                                       void *user_data);

/* Initialise an empty result structure. */
void mariadb_result_init(MariaDBResult *result);

/* Deep clone helper so state machines can take ownership. */
int mariadb_result_clone(MariaDBResult *dest, const MariaDBResult *src);

/* Free every allocation performed by the connector. */
void mariadb_result_free(MariaDBResult *result);

/*
 * Start a non-blocking query. The callback is executed on the libevent loop
 * once the operation finishes (successfully or with an error).
 */
int mariadb_async_query(struct event_base *base,
                        const MariaDBAsyncConfig *config,
                        const char *query,
                        mariadb_async_query_cb callback,
                        void *user_data);


void cleanup_free_mariadb_result(MariaDBResult *result);


#ifdef __cplusplus
}
#endif

#endif /* APP_PLUGINS_DB_MARIADB_ASYNC_H */
