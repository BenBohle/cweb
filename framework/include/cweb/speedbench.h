// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_SPEEDBENCH_H
#define CWEB_SPEEDBENCH_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char path[2048];
    struct timespec start_wall;
    struct timespec end_wall;
    double duration_ms;
} SpeedSample;

void cweb_init_speedbench(void);
void cweb_shutdown_speedbench(void);
void cweb_speedbench_start(const void *key, const char *path);
void cweb_speedbench_end(const void *key);
const SpeedSample *cweb_get_speed_history(size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_SPEEDBENCH_H */