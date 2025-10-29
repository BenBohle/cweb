// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

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