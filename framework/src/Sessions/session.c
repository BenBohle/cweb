#include <cweb/session.h>
#include <cweb/autofree.h>
#include <cweb/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SESSION_STORE_SIZE 1024

static Session* session_store[SESSION_STORE_SIZE];

// Simple djb2 hash function
static unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

void session_store_init() {
	LOG_DEBUG("SESSION", "Initializing session store");
    memset(session_store, 0, sizeof(session_store));

}

static void free_session(Session *s) {
	LOG_DEBUG("SESSION", "Freeing session: %s", s->id);
    if (!s) return;
    for (int i = 0; i < s->data_count; i++) {
        free(s->data[i].key);
        free(s->data[i].value);
    }
    free(s);
}

void session_store_destroy() {
	LOG_DEBUG("SESSION", "Destroying session store");
    for (int i = 0; i < SESSION_STORE_SIZE; i++) {
        Session *current = session_store[i];
        while (current) {
            Session *next = current->next;
            free_session(current);
            current = next;
        }
    }
}

static void generate_session_id(char *id_buf) {
	LOG_DEBUG("SESSION", "Generating new session ID");
    unsigned char random_bytes[SESSION_ID_LEN / 2];
  
    for (size_t i = 0; i < sizeof(random_bytes); i++) {
        sprintf(id_buf + (i * 2), "%02x", random_bytes[i]);
    }
}

Session* get_or_create_session(const char *session_id) {

	LOG_DEBUG("SESSION", "Getting or creating session with ID: %s", session_id ? session_id : "NULL");
    // Trying to find existing session
    if (session_id) {
        unsigned long h = hash(session_id) % SESSION_STORE_SIZE;
        Session *s = session_store[h];
        while (s) {
            if (strcmp(s->id, session_id) == 0) {
                if (time(NULL) < s->expires) {
					LOG_DEBUG("SESSION", "Found valid session with ID: %s", s->id);
                    s->expires = time(NULL) + SESSION_LIFETIME; // Refresh lifetime
                    return s;
                } else {
                    // Session expired, will be removed. Fall through to create new.
					LOG_DEBUG("SESSION", "Session expired: %s", s->id);
                    break;
                }
            }
            s = s->next;
        }
    }

    // Crate a new session
    Session *new_session = calloc(1, sizeof(Session));
	LOG_DEBUG("SESSION", "Allocating new session");
    if (!new_session) {
        return NULL;
    }

    generate_session_id(new_session->id);
	LOG_DEBUG("SESSION", "New session ID generated: %s", new_session->id);
    new_session->expires = time(NULL) + SESSION_LIFETIME;
    
    unsigned long h = hash(new_session->id) % SESSION_STORE_SIZE;
	LOG_DEBUG("SESSION", "Storing session with ID: %s at index: %lu", new_session->id, h);
    new_session->next = session_store[h];
    session_store[h] = new_session;

    return new_session;
}

const char* get_session_value(Session *s, const char *key) {
    if (!s || !key) return NULL;
    for (int i = 0; i < s->data_count; i++) {
        if (strcmp(s->data[i].key, key) == 0) {
            return s->data[i].value;
        }
    }
    return NULL;
}

void set_session_value(Session *s, const char *key, const char *value) {
    if (!s || !key || !value) return;

	LOG_DEBUG("SESSION", "Setting session value: %s = %s", key, value);
    // Update existing key
    for (int i = 0; i < s->data_count; i++) {
        if (strcmp(s->data[i].key, key) == 0) {
            free(s->data[i].value);
            s->data[i].value = strdup(value);
            return;
        }
    }

    // Add new key-value pair
    if (s->data_count < MAX_SESSION_DATA) {
        s->data[s->data_count].key = strdup(key);
        s->data[s->data_count].value = strdup(value);
        s->data_count++;
    }
}
