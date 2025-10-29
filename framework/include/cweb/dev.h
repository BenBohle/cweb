// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_DEV_H
#define CWEB_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CWEB_MODE_PROD = 0,
    CWEB_MODE_DEV  = 1
} CwebMode;

void cweb_set_mode(CwebMode mode);
CwebMode cweb_get_mode(void);


#ifdef __cplusplus
}
#endif

#endif /* CWEB_DEV_H */
