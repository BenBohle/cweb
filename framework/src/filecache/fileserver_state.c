#include "fileserver_internal.h"

CachedFile file_cache[MAX_CACHED_FILES];
int cache_count = 0;
FileServerConfig server_config = {0};
bool initialized = false;
