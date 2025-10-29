// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "commands.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "build.h"
#include "create_command.h"
#include "version_command.h"

#define DEC_MODE_ON "\033(0"
#define DEC_MODE_OFF "\033(B"
#define BOLD "\x1b[1m"
#define RESET "\x1b[0m"
#define CYAN "\x1b[36m"
#define GRAY "\x1b[90m"
#define DARK_GRAY "\x1b[38;5;240m"
#define MID_GRAY "\x1b[38;5;248m"

#define HOR_LINE '\x71'
#define VERT_LINE '\x78'
#define TL_CORNER '\x6C'
#define TR_CORNER '\x6B'
#define BL_CORNER '\x6D'
#define BR_CORNER '\x6A'
#define LEFT_TEE '\x74'
#define RIGHT_TEE '\x75'

typedef struct {
    const char *name;
    const char *description;
    const char *default_value;
    const char *alias;
} cli_option_t;

typedef struct {
    const char *name;
    const char *description;
    const char *usage;
	const char *alias;
    const cli_option_t *options;
    size_t option_count;
} cli_command_t;

typedef struct {
    const char *title;
    const char *description;
    const cli_command_t *commands;
    size_t command_count;
    const cli_option_t *options;
    size_t option_count;
} cli_section_t;

static size_t safe_strlen(const char *value) {
    return value ? strlen(value) : 0;
}

#ifdef _WIN32
static void enable_virtual_terminal(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
static void enable_virtual_terminal(void) {}
#endif

static int get_terminal_width(void) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0) {
        return 80;
    }
    return (int)w.ws_col;
#endif
}

static size_t get_longest_option_length(const cli_option_t *options, size_t count) {
    size_t max_len = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t len = safe_strlen(options[i].name);
        if (len > max_len) {
            max_len = len;
        }
    }
    return max_len;
}

static size_t get_longest_command_length(const cli_command_t *commands, size_t count) {
    size_t max_len = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t name_len = safe_strlen(commands[i].name);
		
        if (name_len > max_len) {
            max_len = name_len;
        }
    }
    return max_len;
}

static int get_max_length(const cli_section_t *sections, size_t count) {
    size_t max_len = 0;

    for (size_t c = 0; c < count; ++c) {
        size_t title_len = safe_strlen(sections[c].title) + 6;
        if (title_len > max_len) {
            max_len = title_len;
        }

        size_t command_gap = get_longest_command_length(sections[c].commands, sections[c].command_count) + 5;
        for (size_t cs = 0; cs < sections[c].command_count; ++cs) {
            const cli_command_t *cmd = &sections[c].commands[cs];
            size_t desc_len = safe_strlen(cmd->description);
			size_t alias_len = cmd->alias ? safe_strlen(cmd->alias) + 11 : 0;
            size_t line_len = command_gap + desc_len + alias_len + 3;
            if (line_len > max_len) {
                max_len = line_len;
            }

            size_t option_gap = get_longest_option_length(cmd->options, cmd->option_count) + 5;
            for (size_t css = 0; css < cmd->option_count; ++css) {
                const cli_option_t *opt = &cmd->options[css];
                size_t opt_desc_len = safe_strlen(opt->description);
                size_t default_len = opt->default_value ? safe_strlen(opt->default_value) + 3 : 0;
                size_t opt_line_len = option_gap + opt_desc_len + default_len + 6;
                if (opt_line_len > max_len) {
                    max_len = opt_line_len;
                }

                if (opt->alias) {
                    size_t alias_len = safe_strlen(opt->alias) + 12;
                    if (alias_len > max_len) {
                        max_len = alias_len;
                    }
                }
            }
        }

        size_t section_opt_gap = get_longest_option_length(sections[c].options, sections[c].option_count) + 5;
        for (size_t cs = 0; cs < sections[c].option_count; ++cs) {
            const cli_option_t *opt = &sections[c].options[cs];
            size_t desc_len = safe_strlen(opt->description);
            size_t default_len = opt->default_value ? safe_strlen(opt->default_value) + 3 : 0;
            size_t opt_line_len = section_opt_gap + desc_len + default_len + 6;
            if (opt_line_len > max_len) {
                max_len = opt_line_len;
            }

            if (opt->alias) {
                size_t alias_len = safe_strlen(opt->alias) + 12;
                if (alias_len > max_len) {
                    max_len = alias_len;
                }
            }
        }
    }

    if (max_len < 40) {
        max_len = 40;
    }

    return (int)max_len;
}

static void draw_option_part(const cli_option_t *opt, size_t gap, size_t width, int is_last) {
    size_t description_len = safe_strlen(opt->description);
    size_t default_len = opt->default_value ? safe_strlen(opt->default_value) + 3 : 0;
    size_t alias_len = safe_strlen(opt->alias);
    int fill = (int)(width - (gap + description_len + default_len + 3));
    if (fill < 0) fill = 0;

    printf(CYAN "%-*s" RESET "%s", (int)gap, opt->name ? opt->name : "", opt->description ? opt->description : "");
    if (opt->default_value) {
        printf(" " GRAY "[%s]" RESET, opt->default_value);
    }

    for (int k = 0; k < fill; ++k) {
        printf(" ");
    }
    printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
    printf("\n");

    if (opt->alias && alias_len > 0) {
        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
        if (is_last == 1) {
			printf(DARK_GRAY DEC_MODE_ON "     %c" DEC_MODE_OFF RESET, BL_CORNER);
			printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, HOR_LINE);
		} else {
			printf(DARK_GRAY DEC_MODE_ON "  %c" DEC_MODE_OFF RESET, VERT_LINE);
			printf(DARK_GRAY DEC_MODE_ON "  %c" DEC_MODE_OFF RESET, BL_CORNER);
			printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, HOR_LINE);
		}
        printf(MID_GRAY "alias: %s" RESET, opt->alias);
        int alias_fill = (int)(width - (alias_len + 12));
        if (alias_fill < 0) alias_fill = 0;
        for (int k = 0; k < alias_fill; ++k) {
            printf(" ");
        }
        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
        printf("\n");
    }
}

static void cli_print_help(const cli_section_t *sections, size_t section_count) {
    int needed_width = get_max_length(sections, section_count);
    int term_width = get_terminal_width();
    int limit = term_width > 0 ? term_width - 2 : needed_width;
    if (limit < 20) {
        limit = 20;
    }

    int width = needed_width;
    if (width > limit) {
        width = limit;
    }
    if (width < 40 && limit >= 40) {
        width = 40;
    }

    enable_virtual_terminal();
    printf("\n");

    for (size_t i = 0; i < section_count; ++i) {
        size_t title_len = safe_strlen(sections[i].title);

        printf(DARK_GRAY DEC_MODE_ON "%c" "%c" DEC_MODE_OFF "[ " RESET, TL_CORNER, HOR_LINE);
        printf(BOLD "%s" RESET, sections[i].title ? sections[i].title : "");
        printf(DARK_GRAY " ]" RESET);
        int padding = width - (int)(title_len + 6);
        if (padding < 0) padding = 0;
        for (int j = 0; j < padding; ++j) {
            printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, HOR_LINE);
        }
        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, TR_CORNER);
        printf("\n");

        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
        for (int j = 0; j < width - 1; ++j) {
            printf(" ");
        }
        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
        printf("\n");

        size_t command_gap = get_longest_command_length(sections[i].commands, sections[i].command_count) + 5;
        for (size_t j = 0; j < sections[i].command_count; ++j) {
            const cli_command_t *cmd = &sections[i].commands[j];
            printf(DARK_GRAY DEC_MODE_ON "%c " DEC_MODE_OFF RESET " ", VERT_LINE);
            printf(BOLD "%-*s" RESET "%s", (int)command_gap, cmd->name ? cmd->name : "", cmd->description ? cmd->description : "");
			if (cmd->alias && safe_strlen(cmd->alias) > 0) {
				printf(" " GRAY "[alias: %s]" RESET, cmd->alias);
			}
            size_t desc_len = safe_strlen(cmd->description);
			size_t alias_len = cmd->alias ? safe_strlen(cmd->alias) + 8 : 0;
            int fill = (int)(width - (command_gap + desc_len + alias_len + 5));
            if (fill < 0) fill = 0;
            for (int k = 0; k < fill; ++k) {
                printf(" ");
            }
            printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
            printf("\n");
            // if (cmd->usage) {
            //     printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
            //     printf("  " MID_GRAY "usage: %s" RESET, cmd->usage);
            //     size_t usage_len = safe_strlen(cmd->usage) + 9;
            //     int usage_fill = (int)(width - usage_len);
            //     if (usage_fill < 0) usage_fill = 0;
            //     for (int k = 0; k < usage_fill; ++k) {
            //         printf(" ");
            //     }
            //     printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
            //     printf("\n");
            // }

            size_t option_gap = get_longest_option_length(cmd->options, cmd->option_count) + 5;
            for (size_t c = 0; c < cmd->option_count; ++c) {
                printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
                printf("  ");
                if (c < cmd->option_count - 1) {
                    printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, LEFT_TEE);
                } else {
                    printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, BL_CORNER);
                }
                printf(DARK_GRAY DEC_MODE_ON "%c " DEC_MODE_OFF RESET, HOR_LINE);
                if (cmd->options) {
                    draw_option_part(&cmd->options[c], option_gap, width - 3, c == cmd->option_count - 1);
                }
            }

            printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
            for (int j = 0; j < width - 1; ++j) {
                printf(" ");
            }
            printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, VERT_LINE);
            printf("\n");
        }

        size_t section_opt_gap = get_longest_option_length(sections[i].options, sections[i].option_count) + 5;
        for (size_t j = 0; j < sections[i].option_count; ++j) {
            printf(DARK_GRAY DEC_MODE_ON "%c " DEC_MODE_OFF RESET " ", VERT_LINE);
            draw_option_part(&sections[i].options[j], section_opt_gap, width, 1);
        }

        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, BL_CORNER);
        for (int j = 0; j < width - 1; ++j) {
            printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, HOR_LINE);
        }
        printf(DARK_GRAY DEC_MODE_ON "%c" DEC_MODE_OFF RESET, BR_CORNER);
        printf("\n\n");
    }
}

static const command_t *const COMMANDS[] = {
    &BUILD_COMMAND,
    &CREATE_COMMAND,
    &VERSION_COMMAND,
    NULL,
};

static void print_command_line(const command_t *cmd) {
    printf("  %-10s %s", cmd->name, cmd->description);
    if (cmd->alias_count > 0) {
        printf(" (aliases: %s", cmd->aliases[0]);
        for (size_t i = 1; i < cmd->alias_count; ++i) {
            printf(", %s", cmd->aliases[i]);
        }
        printf(")");
    }
    printf("\n");
}

const command_t *const *get_commands(void) {
    return COMMANDS;
}

void print_usage(const char *program_name) {
    printf("Usage: %s <command> [options]\n\n", program_name);
    printf("Available commands:\n");

    for (const command_t *const *cmd = COMMANDS; *cmd != NULL; ++cmd) {
        print_command_line(*cmd);
    }

    printf("\nUse '%s help' for more information.\n", program_name);
}

void print_help(void) {
    static const cli_option_t BUILD_OPTIONS[] = {
        {
            .name = "--verbose",
            .description = "Enable verbose output during the build process",
            .default_value = NULL,
            .alias = NULL,
        },
        {
            .name = "--help",
            .description = "Show usage information for the build command",
            .default_value = NULL,
            .alias = "-h",
        },
    };

    static const cli_option_t CREATE_OPTIONS[] = {
        {
            .name = "--help",
            .description = "Display help information for create",
            .default_value = NULL,
            .alias = "-h",
        },
        {
            .name = "--page <name> [directory]",
            .description = "Generate a new page template inside app/routes",
            .default_value = NULL,
            .alias = "-p",
        },
        {
            .name = "--template <name>",
            .description = "Generate a new reusable template stub",
            .default_value = NULL,
            .alias = "-t",
        },
    };

    static const cli_command_t CORE_COMMANDS[] = {
        {
            .name = "build",
            .description = "Build the project and generate sources.conf",
            .usage = "cweb build [options]",
			.alias = "b",
            .options = BUILD_OPTIONS,
            .option_count = ARRAY_SIZE(BUILD_OPTIONS),
        },
        {
            .name = "create",
            .description = "Generate starter files for pages and templates",
            .usage = "cweb create [options]",
			.alias = "new",
            .options = CREATE_OPTIONS,
            .option_count = ARRAY_SIZE(CREATE_OPTIONS),
        },
        {
            .name = "version",
            .description = "Display CLI version information (aliases: --version, -v)",
            .usage = "cweb version",
			.alias = "-v",
            .options = NULL,
            .option_count = 0,
        },
    };

    static const cli_section_t SECTIONS[] = {
        {
            .title = "Core Commands",
            .description = "Core commands for managing your CWeb project",
            .commands = CORE_COMMANDS,
            .command_count = ARRAY_SIZE(CORE_COMMANDS),
            .options = NULL,
            .option_count = 0,
        },
    };

    cli_print_help(SECTIONS, ARRAY_SIZE(SECTIONS));
}

int execute_command(const char *command_name, int argc, char *argv[]) {
    if (!command_name) {
        return -1;
    }

    for (const command_t *const *cmd = COMMANDS; *cmd != NULL; ++cmd) {
        if (matches_alias(command_name, (*cmd)->name, (*cmd)->aliases, (*cmd)->alias_count)) {
            return (*cmd)->handler(argc, argv);
        }
    }

    return -1;
}
