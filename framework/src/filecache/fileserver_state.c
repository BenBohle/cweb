// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "fileserver_internal.h"

CachedFile file_cache[MAX_CACHED_FILES];
int cache_count = 0;
FileServerConfig server_config = {0};
bool initialized = false;
