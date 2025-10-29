// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/compress.h>
#include "compress_internal.h"

static int is_js_ident_char(unsigned char c) {
    return (isalnum(c) || c == '_' || c == '$');
}

inline int js_is_punct(unsigned char c) {
    return kJsPunctuation[c];
}

static int is_regex_prefix(char c) {
    return (c == 0 || c == '(' || c == '=' || c == ':' || c == ',' || c == '?' ||
            c == '!' || c == '&' || c == '|' || c == '^' || c == '%' || c == '+' ||
            c == '-' || c == '~' || c == '{' || c == '}' || c == '[' || c == ';');
}

size_t cweb_minify_js(const char *input, size_t len, char *out) {
    size_t o = 0;
    int in_string = 0;
    char string_quote = '\0';
    int in_regex = 0;
    int in_single_comment = 0;
    int in_multi_comment = 0;
    char prev_sig = 0;

    for (size_t i = 0; i < len; ++i) {
        char c = input[i];
        char next = (i + 1 < len) ? input[i + 1] : '\0';

        if (in_single_comment) {
            if (c == '\n' || c == '\r') {
                in_single_comment = 0;
                if (o && out[o - 1] != '\n') {
                    out[o++] = '\n';
                }
            }
            continue;
        }
        if (in_multi_comment) {
            if (c == '*' && next == '/') {
                in_multi_comment = 0;

                // Nach dem Kommentar: ggf. Separator einfügen, wenn Token sonst zusammenkleben
                int k = (int)i + 2; // Position nach "*/"
                while ((size_t)k < len && cweb_ascii_isspace((unsigned char)input[k])) ++k;
                unsigned char right = (k < (int)len) ? (unsigned char)input[k] : 0;
                if (is_js_ident_char((unsigned char)prev_sig) && is_js_ident_char(right)) {
                    if (o == 0 || out[o - 1] != ' ') {
                        out[o++] = ' ';
                    }
                    prev_sig = ' ';
                }

                ++i; // über '/' springen
            }
            continue;
        }
        if (in_string) {
            out[o++] = c;
            if (c == '\\' && i + 1 < len) {
                out[o++] = input[++i];
            } else if (c == string_quote) {
                in_string = 0;
                prev_sig = string_quote;
            }
            continue;
        }
        if (in_regex) {
            out[o++] = c;
            if (c == '\\' && i + 1 < len) {
                out[o++] = input[++i];
                continue;
            }
            if (c == '/') {
                in_regex = 0;
                prev_sig = '/';
            }
            continue;
        }

        if (c == '\'' || c == '"' || c == '`') {
            in_string = 1;
            string_quote = c;
            out[o++] = c;
            continue;
        }

        if (c == '/' && next == '/') {
            in_single_comment = 1;
            ++i;
            continue;
        }
        if (c == '/' && next == '*') {
            in_multi_comment = 1;
            ++i;
            continue;
        }
        if (c == '/' && is_regex_prefix(prev_sig)) {
            in_regex = 1;
            out[o++] = c;
            prev_sig = 0;
            continue;
        }

        if (cweb_ascii_isspace((unsigned char)c)) {
            size_t k = i + 1;
            while (k < len && cweb_ascii_isspace((unsigned char)input[k])) ++k;
            char next_non = (k < len) ? input[k] : '\0';
            if (!o) {
                i = k - 1;
                continue;
            }
            if (js_is_punct((unsigned char)prev_sig) || js_is_punct((unsigned char)next_non)) {
                i = k - 1;
                continue;
            }
            out[o++] = ' ';
            i = k - 1;
            prev_sig = ' ';
            continue;
        }

        out[o++] = c;
        if (!cweb_ascii_isspace((unsigned char)c)) prev_sig = c;
    }
    while (o && (out[o - 1] == ' ' || out[o - 1] == '\n' || out[o - 1] == '\r' || out[o - 1] == '\t')) {
        --o;
    }
    return o;
}