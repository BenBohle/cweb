#ifndef CWEB_SESSION_H
#define CWEB_SESSION_H

#include <time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_ID_LEN 32
#define MAX_SESSION_DATA 16
#define SESSION_LIFETIME (60 * 30) // 30 minutes

typedef struct {
    char *key;
    char *value;
} SessionData;

typedef struct Session {
    char id[SESSION_ID_LEN + 1];
    time_t expires;
    SessionData data[MAX_SESSION_DATA];
    int data_count;
    struct Session *next; // For hash table chaining
} Session;

// Session store management
void session_store_init();
void session_store_destroy();

// Session operations
Session* get_or_create_session(const char *session_id);
const char* get_session_value(Session *s, const char *key);
void set_session_value(Session *s, const char *key, const char *value);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_SESSION_H */
