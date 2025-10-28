#include <cweb/compress.h>
#include "compress_internal.h"

inline int css_is_punct(unsigned char c) {
    return kCssPunctuation[c];
}

size_t cweb_minify_css(const char *input, size_t len, char *out) {
    size_t o = 0;
    int in_comment = 0;
    int last_space = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = input[i];
        char next = (i + 1 < len) ? input[i + 1] : '\0';
        if (in_comment) {
            if (c == '*' && next == '/') {
                in_comment = 0;

				// Nach dem Kommentar evtl. ein einzelnes Leerzeichen einfügen,
                // wenn sonst Tokens zusammenkleben würden oder ein '+'/'-' Operator folgt.
                // prev = letztes nicht-Whitespace-Zeichen im Output
                char prev = '\0';
                if (o) {
                    size_t p = o;
                    while (p > 0 && cweb_ascii_isspace((unsigned char)out[p - 1])) --p;
                    if (p > 0) prev = out[p - 1];
                }
                // right = nächstes nicht-Whitespace-Zeichen im Input nach "*/"
                int k = (int)i + 2;
                while ((size_t)k < len && cweb_ascii_isspace((unsigned char)input[k])) ++k;
                unsigned char right = (k < (int)len) ? (unsigned char)input[k] : 0;

                int need_space =
                    (!css_is_punct((unsigned char)prev) && !css_is_punct(right)) ||
                    (right == '+' || right == '-') || (prev == '+' || prev == '-');

                if (need_space) {
                    if (o == 0 || out[o - 1] != ' ') {
                        out[o++] = ' ';
                        last_space = 1;
                    }
                }

				
                ++i;
            }
            continue;
        }
        if (c == '/' && next == '*') {
            in_comment = 1;
            ++i;
            continue;
        }
        if (cweb_ascii_isspace((unsigned char)c)) {
            if (last_space) continue;
            char prev = o ? out[o - 1] : '\0';
            size_t k = i + 1;
            char next_non = '\0';
            while (k < len) {
                if (!cweb_ascii_isspace((unsigned char)input[k])) {
                    next_non = input[k];
                    break;
                }
                ++k;
            }
            if (!o || prev == ':' || prev == ',' || prev == ';' || prev == '\n' || prev == '\r' || prev == '\t') {
                continue;
            }
            if (css_is_punct((unsigned char)prev) || css_is_punct((unsigned char)next_non)) {
                continue;
            }
            out[o++] = ' ';
            last_space = 1;
            i = k - 1;
            continue;
        }
        if (c == ':' || c == ';') {
            while (o && (out[o - 1] == ' ' || out[o - 1] == '\n' || out[o - 1] == '\t' || out[o - 1] == '\r')) {
                --o;
            }
        }
        if (last_space && (c == '}' || c == '{' || c == ';' || c == ',')) {
            if (o && out[o - 1] == ' ') --o;
        }
        out[o++] = c;
        last_space = 0;
    }
    return o;
}