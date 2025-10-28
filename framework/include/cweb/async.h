#ifndef CWEB_ASYNC_H
#define CWEB_ASYNC_H

#include <event2/event.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cweb_async_cb)(void *user_data);


int cweb_async(cweb_async_cb cb, void *user_data);


#ifdef __cplusplus
}
#endif

#endif /* CWEB_ASYNC_H */