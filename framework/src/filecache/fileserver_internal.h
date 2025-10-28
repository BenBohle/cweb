#ifndef CWEB_FILESERVER_INTERNAL_H
#define CWEB_FILESERVER_INTERNAL_H

#include <cweb/fileserver.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *extension;
	const char *mime_type;
	int priority;
} MimeMapping;

/* Shared state (defined in fileserver_state.c) */
extern CachedFile file_cache[MAX_CACHED_FILES];
extern int cache_count;
extern FileServerConfig server_config;
extern bool initialized;

/* Lookup */
int scan_directory_recursive(const char *dir_path, const char *base_path);
int update_filechache(const char *path);


/* Read Write utils */
int write_bytes(FILE *f, const void *buf, size_t len);
int read_bytes(FILE *f, void *buf, size_t len);
int write_u32_le(FILE *f, uint32_t v);
int write_u64_le(FILE *f, uint64_t v);
int read_u32_le(FILE *f, uint32_t *out);
int read_u64_le(FILE *f, uint64_t *out);


/* is-er */
bool is_excluded_path(const char *rel_path);
bool is_le(void);
bool cweb_is_file_modified(const char *filepath, time_t cached_time);

/* getters */
int get_resource_priority(const char *filename); // yeet it

extern const MimeMapping mime_mappings[];

#ifdef __cplusplus
}
#endif

#endif /* CWEB_FILESERVER_INTERNAL_H */
