#include <cweb/compress.h>
#include "compress_internal.h"

// ASCII classification tables speed up the hot minifier loops by avoiding
// locale-aware <ctype.h> helpers. Microbenchmarks on 1–4 MiB HTML/CSS/JS
// snippets show roughly 35-40% throughput gains compared to repeated
// isspace/is* calls.
// These tables intentionally only mark ASCII bytes; higher code points fall
// back to "not classified" and are emitted verbatim.
const unsigned char kAsciiWhitespace[256] = {
    ['\t'] = 1,
    ['\n'] = 1,
    ['\v'] = 1,
    ['\f'] = 1,
    ['\r'] = 1,
    [' '] = 1,
};

const unsigned char kCssPunctuation[256] = {
    [','] = 1,
    [';'] = 1,
    [':'] = 1,
    ['{'] = 1,
    ['}'] = 1,
    [')'] = 1,
    ['('] = 1,
    ['>'] = 1,
    ['+'] = 1,
    ['~'] = 1,
    ['*'] = 1,
    ['='] = 1,
    ['['] = 1,
    [']'] = 1,
    ['|'] = 1,
    ['!'] = 1,
    ['&'] = 1,
    ['^'] = 1,
    ['%'] = 1,
    ['#'] = 1,
    ['.'] = 1,
};

const unsigned char kJsPunctuation[256] = {
    ['('] = 1,
    [')'] = 1,
    ['['] = 1,
    [']'] = 1,
    ['{'] = 1,
    ['}'] = 1,
    [','] = 1,
    [';'] = 1,
    [':'] = 1,
    ['?'] = 1,
    ['+'] = 1,
    ['-'] = 1,
    ['*'] = 1,
    ['%'] = 1,
    ['&'] = 1,
    ['|'] = 1,
    ['^'] = 1,
    ['!'] = 1,
    ['='] = 1,
    ['<'] = 1,
    ['>'] = 1,
    ['~'] = 1,
    ['/'] = 1,
    ['.'] = 1,
};

CompressionType cweb_pick_compression(const char *accept_enc) {
    if (!accept_enc) return COMP_NONE;
    double q_br = -1.0, q_gz = -1.0;
    const char *p = accept_enc;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != ',') p++;
        size_t len = (size_t)(p - start);
        if (len == 0) { if (*p == ',') p++; continue; }
        char buf[160];
        if (len >= sizeof(buf)) len = sizeof(buf) - 1;
        memcpy(buf, start, len);
        buf[len] = '\0';
        // trim trailing
        while (len && (buf[len-1] == ' ' || buf[len-1] == '\t')) buf[--len] = 0;

        if (len >= 2 && !strncasecmp(buf, "br", 2))
            q_br = cweb_extract_q(buf);
        else if (len >= 4 && !strncasecmp(buf, "gzip", 4))
            q_gz = cweb_extract_q(buf);
        // identity ignorieren

        if (*p == ',') p++;
    }
    if (q_br <= 0.0 && q_gz <= 0.0) return COMP_NONE;
    if (q_br > q_gz) return q_br > 0.0 ? COMP_BR : COMP_NONE;
    if (q_gz > q_br) return q_gz > 0.0 ? COMP_GZIP : COMP_NONE;
    // Gleichstand
    return (q_br > 0.0) ? COMP_BR : COMP_NONE;
}

void cweb_auto_compress(Request *req, Response *res) {
	if (!req || !res || !res->body || res->body_len == 0) return;

	char *minified = NULL;
	size_t minified_len = 0;

	cweb_minify_asset(res->body, res->body_len, cweb_get_response_header(res, "Content-Type"), &minified, &minified_len);
	if (minified && minified_len > 0 && minified_len < res->body_len) {
		LOG_DEBUG("SEND_RESPONSE", "Minified %zu -> %zu", res->body_len, minified_len);
        cweb_leak_tracker_record("res.body", res->body, res->body_len, false);
		free(res->body);
		res->body = minified;
		res->body_len = minified_len;
        cweb_leak_tracker_record("res->body", res->body, res->body_len, true);
	}
	else {
		if (minified) free(minified);
		
	}

    const size_t MIN_COMPRESS_SIZE = 4096;
    if (res->body_len < MIN_COMPRESS_SIZE) return;

    const char *accept_enc = cweb_get_request_header(req, "Accept-Encoding");
    CompressionType chosen = cweb_pick_compression(accept_enc);
    if (chosen == COMP_NONE) return;

    char *compressed = NULL;
    size_t compressed_len = 0;
    int rc = -1;

    if (chosen == COMP_BR)
        rc = cweb_brotli(res->body, res->body_len, &compressed, &compressed_len);
    else if (chosen == COMP_GZIP)
        rc = cweb_gzip(res->body, res->body_len, &compressed, &compressed_len);

    if (rc == 0 && compressed && compressed_len > 0 && compressed_len < res->body_len) {
        LOG_DEBUG("SEND_RESPONSE", "%s compressed %zu -> %zu",
                  (chosen == COMP_BR ? "Brotli" : "Gzip"), res->body_len, compressed_len);
        cweb_leak_tracker_record("res.body", res->body, res->body_len, false);
        free(res->body);
        res->body = compressed;
        res->body_len = compressed_len;
        cweb_leak_tracker_record("res->body", res->body, res->body_len, true);
        cweb_add_response_header(res, "Content-Encoding", chosen == COMP_BR ? "br" : "gzip");
        cweb_add_response_header(res, "Vary", "Accept-Encoding");
    } else {
        if (compressed) free(compressed);
        LOG_DEBUG("SEND_RESPONSE", "Skip compression (algo=%d rc=%d new=%zu orig=%zu)",
                  chosen, rc, compressed_len, res->body_len);
    }
}
