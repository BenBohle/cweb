#ifndef APP_PLUGINS_ASYNC_STATE_MACHINE_H
#define APP_PLUGINS_ASYNC_STATE_MACHINE_H

#include "db/mariadb_async.h"
#include "fetch/github_fetcher.h"

#include <event2/event.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool has_github;
    bool has_db_result;

    bool fetch_error;
    bool db_error;

    char fetch_error_message[256];
    char db_error_message[256];

    fetch_data_t github;
    MariaDBResult db_result;
} AsyncAggregatedData;

typedef struct AsyncStateMachine AsyncStateMachine;

typedef void (*async_state_machine_completion_cb)(const AsyncAggregatedData *data,
                                                  void *user_data);

struct AsyncStateMachine {
    struct event_base *base;
    async_state_machine_completion_cb completion_cb;
    void *user_data;
    AsyncAggregatedData data;
    size_t pending_count;
    bool completed;
};

void async_state_machine_init(AsyncStateMachine *machine,
                              struct event_base *base,
                              async_state_machine_completion_cb callback,
                              void *user_data);

int async_state_machine_start_github_fetch(AsyncStateMachine *machine,
                                           const GithubFetchConfig *config);

int async_state_machine_start_db_query(AsyncStateMachine *machine,
                                       const MariaDBAsyncConfig *config,
                                       const char *query);

void async_state_machine_cleanup(AsyncStateMachine *machine);

#ifdef __cplusplus
}
#endif

#endif /* APP_PLUGINS_ASYNC_STATE_MACHINE_H */
