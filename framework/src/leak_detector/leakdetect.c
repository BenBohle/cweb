#include <cweb/leak_detector.h>
#include <cweb/logger.h>
#include <cweb/dev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const void *pointer;
    char *name;
    size_t size;
} LeakRecord;

static LeakRecord *g_records = NULL;
static size_t g_record_count = 0;
static size_t g_record_capacity = 0;
static bool g_exit_handler_registered = false;

static void leak_tracker_atexit(void);

static size_t find_record_index(const void *pointer) {
    for (size_t i = 0; i < g_record_count; ++i) {
        if (g_records[i].pointer == pointer) {
            return i;
        }
    }
    return (size_t)-1;
}

static void ensure_capacity(void) {
    if (g_record_count < g_record_capacity) {
        return;
    }

    size_t new_capacity = g_record_capacity == 0 ? 16 : g_record_capacity * 2;
    LeakRecord *new_records = realloc(g_records, new_capacity * sizeof(*new_records));
    if (!new_records) {
        LOG_FATAL("LEAK", "Failed to expand leak tracker storage");
        return;
    }

    g_records = new_records;
    g_record_capacity = new_capacity;
}

static char *duplicate_name(const char *name) {
    if (!name || !*name) {
        return NULL;
    }

    char *copy = strdup(name);
    if (!copy) {
        LOG_ERROR("LEAK", "Failed to duplicate allocation label");
    }
    return copy;
}

static void free_record(LeakRecord *record) {
    if (record->name) {
        free(record->name);
        record->name = NULL;
    }
    record->pointer = NULL;
    record->size = 0;
}

static void register_atexit_handler(void) {
    if (g_exit_handler_registered) {
        return;
    }

    if (atexit(leak_tracker_atexit) == 0) {
        g_exit_handler_registered = true;
    } else {
        LOG_WARNING("LEAK", "Failed to register leak tracker exit handler");
    }
}

void cweb_leak_tracker_record(const char *name, const void *pointer, size_t size, bool is_allocation) {
    if (cweb_get_mode() != CWEB_MODE_DEV) {
        return;
    }

    if (!pointer) {
        return;
    }

    LOG_DEBUG("LEAK", "%s tracking pointer %p (size=%zu) as %s",
              is_allocation ? "Alloc" : "Free",
              pointer,
              size,
              name ? name : "<unnamed>");

    register_atexit_handler();

    size_t index = find_record_index(pointer);

    if (is_allocation) {
        if (index == (size_t)-1) {
            ensure_capacity();
            if (g_record_count < g_record_capacity) {
                g_records[g_record_count].pointer = pointer;
                g_records[g_record_count].size = size;
                g_records[g_record_count].name = duplicate_name(name);
                ++g_record_count;
            }
        } else {
            LeakRecord *record = &g_records[index];
            if (record->name) {
                free(record->name);
            }
            record->name = duplicate_name(name);
            record->size = size;
        }
    } else {
        if (index == (size_t)-1) {
            LOG_WARNING("LEAK", "Attempted to untrack unknown pointer %p (%s)", pointer, name ? name : "<unnamed>");
        } else {
            LeakRecord *record = &g_records[index];
            free_record(record);
            if (index != g_record_count - 1) {
                g_records[index] = g_records[g_record_count - 1];
            }
            --g_record_count;
        }
    }
}

size_t cweb_leak_tracker_outstanding(void) {;
    size_t count = g_record_count;
    return count;
}

void cweb_leak_tracker_dump(void) {
    if (cweb_get_mode() != CWEB_MODE_DEV) {
        return;
    }

    LOG_ERROR("LEAK", "Leak tracker dump requested");
    if (g_record_count == 0) {
        LOG_INFO("LEAK", "No outstanding allocations detected");
        return;
    }

    size_t total_bytes = 0;
    LOG_WARNING("LEAK", "Detected %zu outstanding allocation(s)", g_record_count);
    for (size_t i = 0; i < g_record_count; ++i) {
        const LeakRecord *record = &g_records[i];
        total_bytes += record->size;
        LOG_WARNING("LEAK", " -> ptr=%p size=%zu label=%s",
                    record->pointer,
                    record->size,
                    record->name ? record->name : "<unnamed>");
    }
    LOG_WARNING("LEAK", "Total leaked memory: %zu byte(s)", total_bytes);
}

void cweb_leak_tracker_reset(void) {
    if (cweb_get_mode() != CWEB_MODE_DEV) {
        return;
    }

    for (size_t i = 0; i < g_record_count; ++i) {
        free_record(&g_records[i]);
    }
    free(g_records);
    g_records = NULL;
    g_record_count = 0;
    g_record_capacity = 0;

}

static void leak_tracker_atexit(void) {
    bool has_leaks = g_record_count > 0;

    if (has_leaks) {
        cweb_leak_tracker_dump();
    }

    cweb_leak_tracker_reset();
}