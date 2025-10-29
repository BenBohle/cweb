// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_LEAK_DETECTOR_H
#define CWEB_LEAK_DETECTOR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Records an allocation or deallocation event for leak tracking.
 *
 * When @p is_allocation is true, the given pointer will be tracked with the
 * supplied metadata. When false, the pointer will be removed from the tracker.
 *
 * @param name          Human readable label for the allocation (optional).
 * @param pointer       Address returned by the allocation routine.
 * @param size          Size of the allocation in bytes.
 * @param is_allocation true if the pointer has just been allocated, false if it
 *                      has been released.
 */
void cweb_leak_tracker_record(const char *name, const void *pointer, size_t size, bool is_allocation);

/**
 * @brief Prints a leak report using the framework logger.
 *
 * Call this near shutdown to obtain a list of outstanding allocations. A
 * report is also emitted automatically at process exit when leaks are present
 * for any application that recorded at least one allocation.
 */
void cweb_leak_tracker_dump(void);

/**
 * @brief Returns the number of currently tracked allocations.
 */
size_t cweb_leak_tracker_outstanding(void);

/**
 * @brief Clears all tracked allocations without emitting a report.
 *
 * Primarily intended for test scenarios.
 */
void cweb_leak_tracker_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_LEAK_DETECTOR_H */