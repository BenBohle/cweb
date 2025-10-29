// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef CWEB_AUTOFREE_H
#define CWEB_AUTOFREE_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// These attributes tell the compiler to automatically call the specified
// cleanup function when the variable goes out of scope. This is a powerful
// feature for preventing resource leaks (memory, file descriptors, etc.).
#define AUTOFREE __attribute__((cleanup(cweb_cleanup_free)))
#define AUTOFREE_CLOSE_FILE __attribute__((cleanup(cweb_cleanup_close_file)))
#define AUTOFREE_CLOSE_DIR __attribute__((cleanup(cweb_cleanup_close_dir)))
#define AUTOFREE_CLOSE_SOCKET __attribute__((cleanup(cweb_cleanup_close_socket)))

#define AUTOFREE_PTRPTR __attribute__((cleanup(cweb_cleanup_free_ptrptr)))

// Cleanup function declarations
void cweb_cleanup_free(void *p);
void cweb_cleanup_close_file(FILE **fp);
void cweb_cleanup_close_socket(int *sockfd);
void cweb_cleanup_close_dir(DIR **dp);
void cweb_cleanup_free_ptrptr(void **p);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_AUTOFREE_H */
