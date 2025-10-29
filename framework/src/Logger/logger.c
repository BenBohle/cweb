// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/logger.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <cweb/dev.h>

// A struct to hold the logger's state
typedef struct {
    LogLevel level;
    bool quiet;
    pthread_mutex_t mutex;
} LogContext;

// Global logger context, initialized with default values
static LogContext g_log_context = {
    .level = LOG_LEVEL_DEBUG,
    .quiet = false,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// String representations of log levels
static const char* level_strings[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

// Color representations of log levels
static const char* level_colors[] = {
    CWEB_LOGGER_COLOR_CYAN, CWEB_LOGGER_COLOR_GREEN, CWEB_LOGGER_COLOR_YELLOW, CWEB_LOGGER_COLOR_RED , CWEB_LOGGER_COLOR_MAGENTA
};

void cweb_logger_set_level(LogLevel level) {
    g_log_context.level = level;
}

void cweb_logger_set_quiet(bool quiet) {
    g_log_context.quiet = quiet;
}

void cweb_log_message(LogLevel level, const char *category, const char *file, int line, const char *format, ...) {
    // Suppress log if the level is too low or if in quiet mode
    if (cweb_get_mode() != CWEB_MODE_DEV) {
        return;
    }
    if (level < g_log_context.level || g_log_context.quiet) {
        return;
    }

    // Lock mutex to ensure thread safety
    pthread_mutex_lock(&g_log_context.mutex);

    // Get current time
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", lt);

    // Print the log header to stderr
    fprintf(stderr, "%s [%s%s%s] [%s] ",
            time_buf,
            level_colors[level],
            level_strings[level],
            CWEB_LOGGER_COLOR_RESET,
            category);

    // Print the user's formatted message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    // Print the file and line number for context
    fprintf(stderr, " (%s:%d)\n", file, line);
    fflush(stderr);

    // Unlock the mutex
    pthread_mutex_unlock(&g_log_context.mutex);

    // If the log level is FATAL, terminate the program
    if (level == LOG_LEVEL_FATAL) {
        exit(EXIT_FAILURE);
    }
}
