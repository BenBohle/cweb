// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_LOGGER_H
#define CWEB_LOGGER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


// ANSI color codes for terminal output - can be modified if wished
#ifndef CWEB_LOGGER_COLOR_RESET
#define CWEB_LOGGER_COLOR_RESET   "\x1b[0m"
#endif
#ifndef CWEB_LOGGER_COLOR_RED
#define CWEB_LOGGER_COLOR_RED     "\x1b[31m"
#endif
#ifndef CWEB_LOGGER_COLOR_GREEN
#define CWEB_LOGGER_COLOR_GREEN   "\x1b[32m"
#endif
#ifndef CWEB_LOGGER_COLOR_YELLOW
#define CWEB_LOGGER_COLOR_YELLOW  "\x1b[33m"
#endif
#ifndef CWEB_LOGGER_COLOR_BLUE
#define CWEB_LOGGER_COLOR_BLUE    "\x1b[34m"
#endif
#ifndef CWEB_LOGGER_COLOR_MAGENTA
#define CWEB_LOGGER_COLOR_MAGENTA "\x1b[35m"
#endif
#ifndef CWEB_LOGGER_COLOR_CYAN
#define CWEB_LOGGER_COLOR_CYAN    "\x1b[36m"
#endif

// Log levels enum
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

/**
 * @brief Sets the minimum log level to be displayed.
 *
 * Messages with a level lower than this will be ignored.
 * The default level is LOG_LEVEL_DEBUG.
 *
 * @param level The minimum log level.
 */
void cweb_logger_set_level(LogLevel level);

/**
 * @brief Enables or disables logging entirely.
 *
 * @param quiet If true, all log messages will be suppressed.
 */
void cweb_logger_set_quiet(bool quiet);

/**
 * @brief The core logging function.
 *
 * It is recommended to use the helper macros (LOG_DEBUG, LOG_INFO, etc.)
 * instead of calling this function directly. This function is thread-safe.
 *
 * @param level The log level of the message.
 * @param category The category of the log message (e.g., "DATABASE", "NETWORK").
 * @param file The source file from what the log is issued.
 * @param line The line number in the source file.
 * @param format The printf-style format string for the message.
 * @param ... The arguments for the format string.
 */
void cweb_log_message(LogLevel level, const char *category, const char *file, int line, const char *format, ...);

// Helper macros for convenient logging
#define LOG_DEBUG(category, ...)   cweb_log_message(LOG_LEVEL_DEBUG,   category, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(category, ...)    cweb_log_message(LOG_LEVEL_INFO,    category, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(category, ...) cweb_log_message(LOG_LEVEL_WARNING, category, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(category, ...)   cweb_log_message(LOG_LEVEL_ERROR,   category, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(category, ...)   cweb_log_message(LOG_LEVEL_FATAL,   category, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* CWEB_LOGGER_H */
