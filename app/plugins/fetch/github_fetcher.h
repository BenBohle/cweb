#ifndef APP_PLUGINS_FETCH_GITHUB_FETCHER_H
#define APP_PLUGINS_FETCH_GITHUB_FETCHER_H

#include "../../../build/fetch.template.h"

#include <cweb/fetch.h>

#include <event2/event.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *username;      /* GitHub username to request. */
    const char *user_agent;    /* Optional custom UA header. */
} GithubFetchConfig;

typedef void (*github_fetch_callback)(const fetch_data_t *data,
                                      const char *error_message,
                                      void *user_data);

int github_fetch_user(struct event_base *base,
                      const GithubFetchConfig *config,
                      github_fetch_callback callback,
                      void *user_data);

int github_fetch_data_copy(fetch_data_t *dest, const fetch_data_t *src);
void github_fetch_data_cleanup(fetch_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* APP_PLUGINS_FETCH_GITHUB_FETCHER_H */
