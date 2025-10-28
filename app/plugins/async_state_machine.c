#include "async_state_machine.h"

#include <string.h>

static void async_state_machine_check_completion(AsyncStateMachine *machine) {
    if (!machine || machine->completed) {
        return;
    }

    if (machine->pending_count == 0 && machine->completion_cb) {
        machine->completed = true;
        machine->completion_cb(&machine->data, machine->user_data);
    }
}

void async_state_machine_init(AsyncStateMachine *machine,
                              struct event_base *base,
                              async_state_machine_completion_cb callback,
                              void *user_data) {
    if (!machine) {
        return;
    }

    memset(machine, 0, sizeof(*machine));
    machine->base = base;
    machine->completion_cb = callback;
    machine->user_data = user_data;
    mariadb_result_init(&machine->data.db_result);
}

static void async_state_machine_fetch_cb(const fetch_data_t *data,
                                         const char *error_message,
                                         void *user_data) {
    AsyncStateMachine *machine = user_data;
    if (!machine) {
        return;
    }

    if (data && github_fetch_data_copy(&machine->data.github, data) == 0) {
        machine->data.has_github = true;
    } else {
        machine->data.fetch_error = true;
        if (error_message) {
            strncpy(machine->data.fetch_error_message, error_message,
                    sizeof(machine->data.fetch_error_message) - 1);
        } else {
            strncpy(machine->data.fetch_error_message, "Unknown fetch error",
                    sizeof(machine->data.fetch_error_message) - 1);
        }
    }

    if (machine->pending_count > 0) {
        machine->pending_count--;
    }

    async_state_machine_check_completion(machine);
}

int async_state_machine_start_github_fetch(AsyncStateMachine *machine,
                                           const GithubFetchConfig *config) {
    if (!machine || !config) {
        return -1;
    }

    machine->pending_count++;
    if (github_fetch_user(machine->base, config, async_state_machine_fetch_cb, machine) != 0) {
        machine->pending_count--;
        machine->data.fetch_error = true;
        strncpy(machine->data.fetch_error_message,
                "Unable to start GitHub fetch",
                sizeof(machine->data.fetch_error_message) - 1);
        return -1;
    }

    return 0;
}

static void async_state_machine_db_cb(const MariaDBResult *result,
                                      const char *error_message,
                                      void *user_data) {
    AsyncStateMachine *machine = user_data;
    if (!machine) {
        return;
    }

    if (!error_message && result) {
        if (mariadb_result_clone(&machine->data.db_result, result) == 0) {
            machine->data.has_db_result = true;
        } else {
            machine->data.db_error = true;
            strncpy(machine->data.db_error_message,
                    "Failed to clone MariaDB result",
                    sizeof(machine->data.db_error_message) - 1);
        }
    } else {
        machine->data.db_error = true;
        if (error_message) {
            strncpy(machine->data.db_error_message, error_message,
                    sizeof(machine->data.db_error_message) - 1);
        } else {
            strncpy(machine->data.db_error_message, "Unknown database error",
                    sizeof(machine->data.db_error_message) - 1);
        }
    }

    // Free the original result to prevent memory leaks
    if (result) {
        mariadb_result_free((MariaDBResult *)result);
    }

    if (machine->pending_count > 0) {
        machine->pending_count--;
    }

    async_state_machine_check_completion(machine);
}

int async_state_machine_start_db_query(AsyncStateMachine *machine,
                                       const MariaDBAsyncConfig *config,
                                       const char *query) {
    if (!machine || !config || !query) {
        return -1;
    }

    machine->pending_count++;
    if (mariadb_async_query(machine->base, config, query,
                            async_state_machine_db_cb, machine) != 0) {
        machine->pending_count--;
        machine->data.db_error = true;
        strncpy(machine->data.db_error_message,
                "Unable to start MariaDB query",
                sizeof(machine->data.db_error_message) - 1);
        return -1;
    }

    return 0;
}

void async_state_machine_cleanup(AsyncStateMachine *machine) {
    if (!machine) {
        return;
    }

    if (machine->data.has_github || machine->data.fetch_error) {
        github_fetch_data_cleanup(&machine->data.github);
    }

    if (machine->data.has_db_result || machine->data.db_error) {
        mariadb_result_free(&machine->data.db_result);
    }

    machine->pending_count = 0;
    machine->completed = false;
}
