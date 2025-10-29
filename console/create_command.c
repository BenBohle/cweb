// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "create_command.h"

typedef int (*create_option_handler_t)(int argc, char *argv[]);

typedef struct {
    const char *primary;
    const char *const *aliases;
    size_t alias_count;
    const char *description;
    create_option_handler_t handler;
} create_option_t;

static void print_create_help(void);
static int create_page_template(const char *name, const char *location);
static int show_create_help(int argc, char *argv[]);
static int handle_page_option(int argc, char *argv[]);
static int handle_template_option(int argc, char *argv[]);
int handle_create_command(int argc, char *argv[]);

static const char *const CREATE_ALIASES[] = {"new"};

const command_t CREATE_COMMAND = {
    .name = "create",
    .description = "Generate starter files for pages, templates or components",
    .handler = handle_create_command,
    .aliases = CREATE_ALIASES,
    .alias_count = ARRAY_SIZE(CREATE_ALIASES),
};

static const char *const HELP_ALIASES[] = {"-h"};
static const char *const PAGE_ALIASES[] = {"-p"};
static const char *const TEMPLATE_ALIASES[] = {"-t"};

static const create_option_t CREATE_OPTIONS[] = {
    {
        .primary = "--help",
        .aliases = HELP_ALIASES,
        .alias_count = ARRAY_SIZE(HELP_ALIASES),
        .description = "Display help information",
        .handler = show_create_help,
    },
    {
        .primary = "--page",
        .aliases = PAGE_ALIASES,
        .alias_count = ARRAY_SIZE(PAGE_ALIASES),
        .description = "Generate a new page template",
        .handler = handle_page_option,
    },
    {
        .primary = "--template",
        .aliases = TEMPLATE_ALIASES,
        .alias_count = ARRAY_SIZE(TEMPLATE_ALIASES),
        .description = "Generate a new reusable template",
        .handler = handle_template_option,
    },
};

static const size_t CREATE_OPTION_COUNT = ARRAY_SIZE(CREATE_OPTIONS);

static void print_create_help(void) {
    printf("Usage: create|new [options..]\n");
    printf("Generate new starter files/folder for pages, templates or components.\n\n");
    printf("Options:\n");

    for (size_t i = 0; i < CREATE_OPTION_COUNT; ++i) {
        const create_option_t *option = &CREATE_OPTIONS[i];
        printf("  %s", option->primary);
        if (option->alias_count > 0) {
            printf(", %s", option->aliases[0]);
            for (size_t j = 1; j < option->alias_count; ++j) {
                printf(", %s", option->aliases[j]);
            }
        }
        printf("\n      %s\n", option->description);
    }
}

static int create_page_template(const char *name, const char *location) {
    if (name == NULL) {
        printf("Error: Page name cannot be NULL.\n");
        return -1;
    }

    if (location == NULL) {
        location = "app/routes";
    }

    if (strlen(name) == 0) {
        printf("Error: Page name cannot be empty.\n");
        return -1;
    }

    AUTOFREE char *full_path = malloc(strlen(location) + strlen(name) + 2);
    if (full_path == NULL) {
        printf("Error: Memory allocation failed.\n");
        return -1;
    }
    snprintf(full_path, strlen(location) + strlen(name) + 2, "%s/%s", location, name);

    if (location != NULL && !is_directory(location)) {
        printf("Error: Invalid directory '%s'.\n", location);
        return -1;
    }

    if (create_directory_if_not_exists(full_path) != 0) {
        return 1;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s.page.c", full_path, name);
    printf("Creating page template file: %s\n", file_path);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        printf("Error: Could not create page template file '%s'.\n", file_path);
        return -1;
    }

    fprintf(file, "#include \"%s.page.h\"\n\n", name);
    fprintf(file, "void %s_%s(Request *req, Response *res) {\n\n", name, "page");
    fprintf(file, " if (strcmp(req->method, \"POST\") == 0) { \n");
    fprintf(file, "         res->status_code = 403;\n");
    fprintf(file, "         res->body = \"Method not allowed\";\n");
    fprintf(file, "         res->body_len = strlen(res->body);\n");
    fprintf(file, "         cweb_add_response_header(res, \"Content-Type\", \"text/plain\");\n");
    fprintf(file, "         return;\n");
    fprintf(file, " }\n\n");

    fprintf(file, "         res->status_code = 200;\n");
    fprintf(file, "         res->body = \"Hello\";\n");
    fprintf(file, "         res->body_len = strlen(res->body);\n");
    fprintf(file, "         cweb_add_response_header(res, \"Content-Type\", \"text/plain\");\n");
    fprintf(file, "         return;\n");
    fprintf(file, "}\n\n");

    fclose(file);

    file_path[0] = '\0';
    snprintf(file_path, sizeof(file_path), "%s/%s.page.h", full_path, name);

    file = fopen(file_path, "w");
    if (file == NULL) {
        printf("Error: Could not create page template file '%s'.\n", file_path);
        return -1;
    }

    char guard_name[256];
    snprintf(guard_name, sizeof(guard_name), "%s_PAGE_H", name);

    for (int i = 0; guard_name[i]; i++) {
        if (guard_name[i] == '.') {
            guard_name[i] = '_';
        }
        guard_name[i] = toupper((unsigned char)guard_name[i]);
    }

    fprintf(file, "#ifndef %s\n", guard_name);
    fprintf(file, "#define %s\n\n", guard_name);
    fprintf(file, "#include <cweb/cweb.h>\n\n\n");
    fprintf(file, "void %s_%s(Request *req, Response *res);\n\n", name, "page");
    fprintf(file, "#endif // %s\n", guard_name);

    fclose(file);

    printf("Page template .h '%s' created successfully at '%s'.\n", name, file_path);
    return 0;
}

static int show_create_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    print_create_help();
    return 0;
}

static int handle_page_option(int argc, char *argv[]) {
    const char *name = argc > 2 ? argv[2] : NULL;
    if (!name) {
        printf("Error: Missing name for page template.\n");
        return 1;
    }

    const char *location = argc > 3 ? argv[3] : NULL;
    printf("Generating new page template: %s\n", name);
    return create_page_template(name, location);
}

static int handle_template_option(int argc, char *argv[]) {
    const char *name = argc > 2 ? argv[2] : NULL;
    if (!name) {
        printf("Error: Missing name for template.\n");
        return 1;
    }

    printf("Not implemented. Just create a .cweb in templates :) %s\n", name);
    return 0;
}

int handle_create_command(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: No options provided.\n");
        print_create_help();
        return 1;
    }

    const char *option = argv[1];
    for (size_t i = 0; i < CREATE_OPTION_COUNT; ++i) {
        const create_option_t *spec = &CREATE_OPTIONS[i];
        if (matches_alias(option, spec->primary, spec->aliases, spec->alias_count)) {
            return spec->handler(argc, argv);
        }
    }

    printf("Error: Unknown option '%s'.\n", option);
    print_create_help();
    return 1;
}
