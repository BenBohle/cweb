#ifndef SECURITY_H
#define SECURITY_H

#include "common.h"

int validate_path(const char *path);
int sanitize_filename(const char *filename, char *output, size_t output_size);

#endif // SECURITY_H
