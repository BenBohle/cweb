// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/speedbench.h>
#include <string.h>

#define SPEED_BENCHMARK_MAX_ACTIVE 256
#define SPEED_BENCHMARK_HISTORY_CAPACITY 2048
#define SPEED_BENCHMARK_PATH_MAX 2048

typedef struct {
    int in_use;
    const void *key;
    char path[SPEED_BENCHMARK_PATH_MAX];
    struct timespec start_mono;
    struct timespec start_wall;
} ActiveTimer;

static ActiveTimer active_timers[SPEED_BENCHMARK_MAX_ACTIVE];
static SpeedSample history_buffer[SPEED_BENCHMARK_HISTORY_CAPACITY];
static SpeedSample history_snapshot[SPEED_BENCHMARK_HISTORY_CAPACITY];
static size_t history_size = 0;
static size_t history_next_index = 0;

static clockid_t benchmark_clock(void) {
#ifdef CLOCK_MONOTONIC_RAW
    return CLOCK_MONOTONIC_RAW;
#else
    return CLOCK_MONOTONIC;
#endif
}

static double diff_ms(const struct timespec *start, const struct timespec *end) {
    long sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    if (nsec < 0) {
        --sec;
        nsec += 1000000000L;
    }
    return (double)sec * 1000.0 + (double)nsec / 1000000.0;
}

static ActiveTimer *find_timer_slot(const void *key) {
    for (size_t i = 0; i < SPEED_BENCHMARK_MAX_ACTIVE; ++i) {
        if (active_timers[i].in_use && active_timers[i].key == key) {
            return &active_timers[i];
        }
    }
    return NULL;
}

static ActiveTimer *get_free_timer_slot(void) {
    for (size_t i = 0; i < SPEED_BENCHMARK_MAX_ACTIVE; ++i) {
        if (!active_timers[i].in_use) {
            return &active_timers[i];
        }
    }
    return NULL;
}

void cweb_init_speedbench(void) {
    memset(active_timers, 0, sizeof(active_timers));
    memset(history_buffer, 0, sizeof(history_buffer));
    memset(history_snapshot, 0, sizeof(history_snapshot));
    history_size = 0;
    history_next_index = 0;
}

void cweb_shutdown_speedbench(void) {
    cweb_init_speedbench();
}

void cweb_speedbench_start(const void *key, const char *path) {
    if (!key || !path) {
        return;
    }

    ActiveTimer *slot = find_timer_slot(key);
    if (!slot) {
        slot = get_free_timer_slot();
    }

    if (!slot) {
        return;
    }

    struct timespec start_mono;
    struct timespec start_wall;

    if (clock_gettime(benchmark_clock(), &start_mono) != 0) {
        return;
    }

    if (clock_gettime(CLOCK_REALTIME, &start_wall) != 0) {
        return;
    }

    slot->in_use = 1;
    slot->key = key;
    strncpy(slot->path, path, SPEED_BENCHMARK_PATH_MAX - 1);
    slot->path[SPEED_BENCHMARK_PATH_MAX - 1] = '\0';
    slot->start_mono = start_mono;
    slot->start_wall = start_wall;
}

void cweb_speedbench_end(const void *key) {
    if (!key) {
        return;
    }

    ActiveTimer *slot = find_timer_slot(key);
    if (!slot) {
        return;
    }

    struct timespec end_mono;
    struct timespec end_wall;

    if (clock_gettime(benchmark_clock(), &end_mono) != 0) {
        return;
    }

    if (clock_gettime(CLOCK_REALTIME, &end_wall) != 0) {
        return;
    }

    SpeedSample sample;
    strncpy(sample.path, slot->path, SPEED_BENCHMARK_PATH_MAX);
    sample.path[SPEED_BENCHMARK_PATH_MAX - 1] = '\0';
    sample.start_wall = slot->start_wall;
    sample.end_wall = end_wall;
    sample.duration_ms = diff_ms(&slot->start_mono, &end_mono);

    history_buffer[history_next_index] = sample;
    history_next_index = (history_next_index + 1) % SPEED_BENCHMARK_HISTORY_CAPACITY;
    if (history_size < SPEED_BENCHMARK_HISTORY_CAPACITY) {
        history_size++;
    }

    slot->in_use = 0;
    slot->key = NULL;
    slot->path[0] = '\0';
}

const SpeedSample *cweb_get_speed_history(size_t *out_count) {
    size_t count = history_size;
    if (count == 0) {
        if (out_count) {
            *out_count = 0;
        }
        return NULL;
    }

    size_t start_index = (history_size == SPEED_BENCHMARK_HISTORY_CAPACITY) ? history_next_index : 0;
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (start_index + i) % SPEED_BENCHMARK_HISTORY_CAPACITY;
        history_snapshot[i] = history_buffer[idx];
    }

    if (out_count) {
        *out_count = count;
    }

    return history_snapshot;
}