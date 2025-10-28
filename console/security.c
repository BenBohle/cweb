#include "security.h"

int validate_path(const char *path) {
    if (!path || strlen(path) == 0) {
        fprintf(stderr, "Error: Empty path provided\n");
        return -1;
    }
    
    if (strlen(path) >= MAX_PATH_LEN) {
        fprintf(stderr, "Error: Path too long\n");
        return -1;
    }
    
    // Check for directory traversal attempts
    if (strstr(path, "..") != NULL) {
        fprintf(stderr, "Error: Directory traversal not allowed\n");
        return -1;
    }
    
    // Check for null bytes (security)
    if (strlen(path) != strcspn(path, "\0")) {
        fprintf(stderr, "Error: Invalid characters in path\n");
        return -1;
    }
    
    return 0;
}

int sanitize_filename(const char *filename, char *output, size_t output_size) {
    if (!filename || !output || output_size == 0) {
        return -1;
    }
    
    size_t len = strlen(filename);
    if (len >= output_size) {
        return -1;
    }
    
    // Copy filename, replacing dangerous characters
    for (size_t i = 0; i < len && i < output_size - 1; i++) {
        char c = filename[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || 
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            output[i] = '_';
        } else {
            output[i] = c;
        }
    }
    output[len] = '\0';
    
    return 0;
}
