// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <cweb/codegen.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cweb/leak_detector.h>

char* cweb_escape_html(const char *input) {
    if (!input) return NULL;
    
    size_t input_len = strlen(input);
    size_t output_len = input_len * 6 + 1; // Worst case: all chars need escaping??
    char *output = malloc(output_len);
    if (!output) return NULL;
    
    size_t out_pos = 0;
    for (size_t i = 0; i < input_len && out_pos < output_len - 6; i++) {
        switch (input[i]) {
            case '<':
                strcpy(output + out_pos, "&lt;");
                out_pos += 4;
                break;
            case '>':
                strcpy(output + out_pos, "&gt;");
                out_pos += 4;
                break;
            case '&':
                strcpy(output + out_pos, "&amp;");
                out_pos += 5;
                break;
            case '"':
                strcpy(output + out_pos, "&quot;");
                out_pos += 6;
                break;
            case '\'':
                strcpy(output + out_pos, "&#x27;");
                out_pos += 6;
                break;
            default:
                output[out_pos++] = input[i];
                break;
        }
    }
    cweb_leak_tracker_record("escape_html_outpup", output, output_len, true);
    
    output[out_pos] = '\0';
    return output;
}

bool cweb_validate_tag_name(const char *tag) {
    if (!tag || strlen(tag) == 0) return false;
    
    // Tag name must start with letter
    if (!isalpha(tag[0])) return false;
    
    // Rest can be letters, numbers, or underscores
    for (size_t i = 1; i < strlen(tag); i++) {
        if (!isalnum(tag[i]) && tag[i] != '_') {
            return false;
        }
    }
    
    return true;
}

bool cweb_sanitize_c_code(const char *code) {
    if (!code) return false;
    
	// why did i need that
    const char *dangerous[] = {
        "system(", "exec(", "popen(", "fork(", 
        "unlink(", "remove(", "rmdir(",
        "#include", "#define", "#pragma",
        NULL
    };
    
    for (int i = 0; dangerous[i]; i++) {
        if (strstr(code, dangerous[i])) {
            return false;
        }
    }
    
    return true;
}
