#include <cweb/compress.h>
#include "compress_internal.h"

int cweb_ascii_isspace(unsigned char c) {
    return kAsciiWhitespace[c];
}

int cweb_contains_token_ci(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return 0;
    size_t nlen = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < nlen && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) {
            ++i;
        }
        if (i == nlen) return 1;
    }
    return 0;
}

double cweb_extract_q(const char *token) {
    const char *q = strstr(token, "q=");
    if (!q) return 1.0;
    q += 2;
    char *end = NULL;
    double v = strtod(q, &end);
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return v;
}