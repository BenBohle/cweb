#ifndef CREATE_COMMAND_H
#define CREATE_COMMAND_H

#include "common.h"
#include "file_utils.h"
#include "security.h"
#include <cweb/template.h>
#include <cweb/autofree.h>

int handle_create_command(int argc, char *argv[]);

extern const command_t CREATE_COMMAND;

#endif // CREATE_COMMAND_H
