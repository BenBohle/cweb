// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_CWEB_H
#define CWEB_CWEB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cweb/server.h>
#include <cweb/routing.h>
#include <cweb/http.h>
#include <cweb/autofree.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cweb/logger.h>
#include <cweb/fileserver.h>
#include <cweb/template.h>
#include <cweb/fetch.h>
#include <cweb/speedbench.h>
#include <cweb/cwagger.h>

/* public helpers */
void cweb_register_frontend_asset_function(void (*func)(void));
void cweb_call_all_frontend_asset_functions(void);

#define REGISTER_FRONTEND_ASSET(func) \
    __attribute__((constructor)) static void auto_register_##func(void) { cweb_register_frontend_asset_function(func); }

#ifdef __cplusplus
}
#endif

#endif /* CWEB_CWEB_H */
