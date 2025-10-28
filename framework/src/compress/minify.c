#include <cweb/compress.h>
#include "compress_internal.h"

static size_t minify_copy(const char *input, size_t len, char *out) {
    memcpy(out, input, len);
    return len;
}

static MinifyKind detect_minify_kind(const char *content_type) {
    if (!content_type) return MINIFY_KIND_NONE;
    if (cweb_contains_token_ci(content_type, "text/html") ||
        cweb_contains_token_ci(content_type, "application/xhtml")) {
        return MINIFY_KIND_HTML;
    }
    if (cweb_contains_token_ci(content_type, "text/css")) {
        return MINIFY_KIND_CSS;
    }
    if (cweb_contains_token_ci(content_type, "javascript")) {
        return MINIFY_KIND_JS;
    }
    return MINIFY_KIND_NONE;
}

// Public API to minify an asset based on its content type
int cweb_minify_asset(const char *input,
                 size_t input_len,
                 const char *content_type,
                 char **output,
                 size_t *output_len) {
    if (!output || !output_len || !input) {
        return -1;
    }

    *output = NULL;
    *output_len = 0;

    if (input_len == 0) {
        return 0;
    }

    MinifyKind kind = detect_minify_kind(content_type);
    size_t buffer_len = input_len + 1;
    char *buffer = (char *)malloc(buffer_len);
    
    if (!buffer) {
        LOG_ERROR("COMPRESS", "Allocation failed for minify buffer");
        return -1;
    }

    size_t produced = 0;
    switch (kind) {
        case MINIFY_KIND_HTML:
            produced = cweb_minify_html(input, input_len, buffer);
            break;
        case MINIFY_KIND_CSS:
            produced = cweb_minify_css(input, input_len, buffer);
            break;
        case MINIFY_KIND_JS:
            produced = cweb_minify_js(input, input_len, buffer);
            break;
        case MINIFY_KIND_NONE:
        default:
            produced = minify_copy(input, input_len, buffer);
            break;
    }

    if (produced == 0 && input_len > 0) {
        produced = minify_copy(input, input_len, buffer);
    }

    buffer[produced] = '\0';
    *output = buffer;
    *output_len = produced;

    LOG_DEBUG("COMPRESS", "Minify (%d) %zu -> %zu", (int)kind, input_len, produced);
    return 0;
}
