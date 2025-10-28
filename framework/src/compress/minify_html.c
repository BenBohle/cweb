#include <cweb/compress.h>
#include "compress_internal.h"

static int is_preserve_tag(const char *name, size_t len) {
    if (len == 0) return 0;
    if (len == 3 && !strncasecmp(name, "pre", len)) return 1;
    if (len == 8 && !strncasecmp(name, "textarea", len)) return 1;
    if (len == 6 && !strncasecmp(name, "script", len)) return 1;
    if (len == 5 && !strncasecmp(name, "style", len)) return 1;
    return 0;
}

size_t cweb_minify_html(const char *input, size_t len, char *out) {
    size_t o = 0;
    int preserve_depth = 0;

    for (size_t i = 0; i < len; ++i) {
        char c = input[i];

        if (c == '<') {
            if (preserve_depth == 0 && i + 3 < len && input[i + 1] == '!' && input[i + 2] == '-' && input[i + 3] == '-') {
                i += 4;
                while (i + 2 < len && !(input[i] == '-' && input[i + 1] == '-' && input[i + 2] == '>')) {
                    ++i;
                }
                if (i + 2 < len) i += 2;
                continue;
            }

            size_t tag_start = i;
            size_t j = i + 1;
            int closing = 0;
            if (j < len && input[j] == '/') {
                closing = 1;
                ++j;
            }
            while (j < len && cweb_ascii_isspace((unsigned char)input[j])) ++j;
            size_t name_start = j;
            while (j < len && (isalnum((unsigned char)input[j]) || input[j] == '-' || input[j] == ':')) ++j;
            size_t name_len = j - name_start;
            int preserve_tag = name_len ? is_preserve_tag(&input[name_start], name_len) : 0;

            if (preserve_depth > 0 && !preserve_tag && !closing) {
                out[o++] = c;
                continue;
            }

            size_t k = tag_start;
            int in_quote = 0;
            char quote = '\0';
            int wrote_space = 0;
            for (; k < len; ++k) {
                char ch = input[k];
                out[o++] = ch;
                if (ch == '\'' || ch == '"') {
                    if (in_quote && quote == ch) {
                        in_quote = 0;
                    } else if (!in_quote) {
                        in_quote = 1;
                        quote = ch;
                    }
                    wrote_space = 0;
                } else if (!in_quote && cweb_ascii_isspace((unsigned char)ch)) {
                    if (wrote_space) {
                        --o;
                        continue;
                    }
                    wrote_space = 1;
                    out[o - 1] = ' ';
                } else if (!in_quote && ch == '>') {
                    if (closing && preserve_tag && preserve_depth > 0) {
                        --preserve_depth;
                    } else if (!closing && preserve_tag) {
                        ++preserve_depth;
                    }
                    break;
                } else if (!cweb_ascii_isspace((unsigned char)ch)) {
                    wrote_space = 0;
                }
            }
            i = k;
            continue;
        }

        if (preserve_depth > 0) {
            out[o++] = c;
            continue;
        }

        if (cweb_ascii_isspace((unsigned char)c)) {
            size_t k = i + 1;
            while (k < len && cweb_ascii_isspace((unsigned char)input[k])) ++k;
            char prev = o ? out[o - 1] : '\0';
            char next = (k < len) ? input[k] : '\0';
            if (prev == '>' || prev == '\0' || next == '<' || next == '\0') {
                i = k - 1;
                continue;
            }
            out[o++] = ' ';
            i = k - 1;
            continue;
        }

        out[o++] = c;
    }

    while (o && cweb_ascii_isspace((unsigned char)out[o - 1])) --o;
    return o;
}