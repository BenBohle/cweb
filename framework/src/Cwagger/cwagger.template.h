// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef _CWAGGER_TEMPLATE_H_H
#define _CWAGGER_TEMPLATE_H_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Page data structure for the example template
#include <cweb/cwagger.h>

char* cwagger_template(cwagger_endpoint *data, size_t count);

#endif // _CWAGGER_TEMPLATE_H_H
