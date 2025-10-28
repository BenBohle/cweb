#include "codegen_internal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool extract_template_signature(const char *template_content, template_signature_t *signature) {
    if (!template_content || !signature) {
        return false;
    }

    memset(signature, 0, sizeof(*signature));

    const char *header_marker = "<%header";
    const char *header_start = strstr(template_content, header_marker);
    if (!header_start) {
        return false;
    }

    const char *header_end = strstr(header_start, "%>");
    if (!header_end) {
        return false;
    }

    const char *content_start = header_start;
    const char *newline = strchr(header_start, '\n');
    if (newline && newline < header_end) {
        content_start = newline + 1;
    } else {
        content_start = header_start + strlen(header_marker);
        while (content_start < header_end && isspace((unsigned char)*content_start)) {
            content_start++;
        }
    }

    signature->header_content_start = content_start;
    signature->header_end = header_end;
    signature->header_content_length = (size_t)(header_end - content_start);

    if (signature->header_content_length == 0) {
        return false;
    }

    char *header_copy = malloc(signature->header_content_length + 1);
    if (!header_copy) {
        return false;
    }

    memcpy(header_copy, content_start, signature->header_content_length);
    header_copy[signature->header_content_length] = '\0';

    bool found = false;
    const char *search = header_copy;
    while (*search) {
        const char *char_pos = strstr(search, "char");
        if (!char_pos) {
            break;
        }

        const char *p = char_pos + 4;
        while (isspace((unsigned char)*p)) {
            p++;
        }

        if (*p != '*') {
            search = char_pos + 4;
            continue;
        }

        p++;
        while (isspace((unsigned char)*p)) {
            p++;
        }

        const char *name_start = p;
        while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
            p++;
        }

        size_t name_len = (size_t)(p - name_start);
        if (name_len == 0 || name_len >= sizeof(signature->func_name)) {
            search = char_pos + 4;
            continue;
        }

        while (isspace((unsigned char)*p)) {
            p++;
        }

        if (*p != '(') {
            search = char_pos + 4;
            continue;
        }

        p++;
        const char *param_start = p;
        int depth = 1;
        while (*p && depth > 0) {
            if (*p == '(') {
                depth++;
            } else if (*p == ')') {
                depth--;
            }
            p++;
        }

        if (depth != 0) {
            break;
        }

        const char *param_end = p - 1;
        while (param_start < param_end && isspace((unsigned char)*param_start)) {
            param_start++;
        }
        while (param_end > param_start && isspace((unsigned char)*(param_end - 1))) {
            param_end--;
        }

        strncpy(signature->func_name, name_start, name_len);
        signature->func_name[name_len] = '\0';

        size_t param_len = (size_t)(param_end - param_start);
        if (param_len >= sizeof(signature->param_signature)) {
            param_len = sizeof(signature->param_signature) - 1;
        }
        if (param_len > 0) {
            memcpy(signature->param_signature, param_start, param_len);
        }
        signature->param_signature[param_len] = '\0';

        found = true;
        break;
    }

    free(header_copy);
    return found;
}

bool header_contains_css_declaration(const template_signature_t *signature) {
    if (!signature || !signature->header_content_start || signature->header_content_length == 0) {
        return false;
    }

    if (signature->func_name[0] == '\0') {
        return false;
    }

    char needle[256];
    snprintf(needle, sizeof(needle), "%s_css", signature->func_name);
    size_t needle_len = strlen(needle);

    if (needle_len == 0) {
        return false;
    }

    const char *start = signature->header_content_start;
    size_t available = signature->header_content_length;
    for (size_t i = 0; i + needle_len <= available; ++i) {
        if (strncmp(start + i, needle, needle_len) == 0) {
            return true;
        }
    }

    return false;
}

const char* template_body_start(const template_signature_t *signature, const char *template_content) {
    if (signature && signature->header_end) {
        return signature->header_end + 2;
    }
    return template_content;
}
