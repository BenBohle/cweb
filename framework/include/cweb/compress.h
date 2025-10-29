// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef CWEB_COMPRESS_H
#define CWEB_COMPRESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <cweb/http.h>


typedef enum {
    COMP_NONE = 0,
    COMP_BR,
    COMP_GZIP
} CompressionType;

/**
 * Compresses the given input string using Brotli.
 * @param input The input string to compress.
 * @param input_len The length of the input string.
 * @param output A pointer to the compressed output buffer (allocated internally).
 * @param output_len A pointer to store the length of the compressed output.
 * @return 0 on success, non-zero on failure.
 */
int cweb_brotli(const char* input, size_t input_len, char** output, size_t* output_len);

/**
 * Compresses the given input string using Gzip.
 * @param input The input string to compress.
 * @param input_len The length of the input string.
 * @param output A pointer to the compressed output buffer (allocated internally).
 * @param output_len A pointer to store the length of the compressed output.
 * @return 0 on success, non-zero on failure.
 */
int cweb_gzip(const char* input, size_t input_len, char** output, size_t* output_len);
CompressionType cweb_pick_compression(const char *accept_enc);
void cweb_auto_compress(Request *req, Response *res);
double cweb_extract_q(const char *token);
int cweb_minify_asset(const char *input,
    size_t input_len,
    const char *content_type,
    char **output,
    size_t *output_len);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_COMPRESS_H */
