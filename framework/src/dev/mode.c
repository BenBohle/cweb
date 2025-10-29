// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/dev.h>

static CwebMode current_mode = CWEB_MODE_PROD;

void cweb_set_mode(CwebMode mode) {
    current_mode = mode;
}

CwebMode cweb_get_mode(void) {
    return current_mode;
}