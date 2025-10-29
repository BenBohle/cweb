// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "build.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

int handle_build_command(int argc, char *argv[]);

static const char *const BUILD_ALIASES[] = {"b"};

const command_t BUILD_COMMAND = {
    .name = "build",
    .description = "Build the cweb project by compiling templates and generating sources.conf",
    .handler = handle_build_command,
    .aliases = BUILD_ALIASES,
    .alias_count = ARRAY_SIZE(BUILD_ALIASES),
};

int cweb_compile(const char *file_path) {

	if (validate_path(file_path) != 0) {
        return -1;
    }
    
    printf("Compiling: %s\n", file_path);
    
    AUTOFREE char *filename = strdup(file_path);
    if (!filename) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }
    
    char *dot = strrchr(filename, '.');
    if (dot) *dot = '\0';
    
    char *basename = strrchr(filename, '/');
    basename = basename ? basename + 1 : filename;
    
    // TODO: entfernen weil wird doch hier net gebraucht oder?
    // Sanitize basename for security
    char safe_basename[256];
    if (sanitize_filename(basename, safe_basename, sizeof(safe_basename)) != 0) {
        fprintf(stderr, "Error: Invalid filename '%s'\n", basename);
        return -1;
    }
    
    // Create output path in build verzeichnis
    char output_path[MAX_PATH_LEN];
    int ret = snprintf(output_path, sizeof(output_path), "build/%s", safe_basename);
    if (ret >= (int)sizeof(output_path) || ret < 0) {
        fprintf(stderr, "Error: Output path too long\n");
        return -1;
    }

    if (!cweb_compile_template(file_path, output_path)) {
        fprintf(stderr, "Compilation failed!\n");
        return -1;
    }

	printf("Generated: %s\n", output_path);
	return 0;
}

int combine_file_lists(file_list_t *result, size_t num_lists, ...) {
    if (!result || num_lists == 0) {
        // LOG_ERROR("BUILD", "Invalid arguments to combine_file_lists_var");
        return -1;
    }
    
    size_t total_files = 0;
    va_list args;
    va_start(args, num_lists);

    // Zähle alle Dateien
    for (size_t i = 0; i < num_lists; i++) {
        const file_list_t *lst = va_arg(args, const file_list_t*);
        if (!lst) continue;
        total_files += lst->count;
    }
    va_end(args);
    
    if (total_files > MAX_FILES) {
        fprintf(stderr, "Error: Too many files to combine (limit: %d)\n", MAX_FILES);
        result->files = NULL;
        result->count = 0;
        result->capacity = 0;
        return -1;
    }

    if (total_files == 0) {
        result->files = NULL;
        result->count = 0;
        result->capacity = 0;
        // LOG_DEBUG("BUILD", "No files to combine");
        return 0;
    }
    
    result->files = malloc(total_files * sizeof(char*));
    if (!result->files) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }
    result->capacity = total_files;
    result->count = 0;
    
    va_start(args, num_lists);
    for (size_t i = 0; i < num_lists; i++) {
        const file_list_t *lst = va_arg(args, const file_list_t*);
        if (!lst) continue;
        for (size_t j = 0; j < lst->count; j++) {
            result->files[result->count] = strdup(lst->files[j]);
            if (!result->files[result->count]) {
                // LOG_ERROR("BUILD", "Memory allocation failed for file string");
                cleanup_file_list(result);
                va_end(args);
                return -1;
            }
            result->count++;
        }
    }
    va_end(args);
    
    return 0;
}

int write_build_h(size_t num_lists, ...) {
    if (num_lists == 0) {
        // LOG_ERROR("BUILD", "Invalid arguments to write_build_h");
        return -1;
    }

    size_t total_files = 0;
    va_list args;
    va_start(args, num_lists);

    // Zähle alle Dateien
    for (size_t i = 0; i < num_lists; i++) {
        const file_list_t *lst = va_arg(args, const file_list_t*);
        if (!lst) continue;
        total_files += lst->count;
    }
    va_end(args);

    if (total_files == 0) {
        // LOG_DEBUG("BUILD", "No files to write to build.h");
        return 0;
    }

    FILE *headerfile = fopen("build/build.h", "w");
    if (!headerfile) {
        fprintf(stderr, "Error: Cannot create build.h: %s\n", strerror(errno));
        return -1;
    }

    fprintf(headerfile, "// Auto-generated build.h\n\n");

    va_start(args, num_lists);
    for (size_t i = 0; i < num_lists; i++) {
        const file_list_t *lst = va_arg(args, const file_list_t*);
        if (!lst) continue;
        for (size_t j = 0; j < lst->count; j++) {
            if (!lst->files[j]) {
			    continue; // Skip null entries
		    }
            if (strcmp(lst->files[j], "build/build.h") != 0) {
                if (strstr(lst->files[j], "build/") == lst->files[j]) {
                    // If the file is in the build directory, include it directly
                    fprintf(headerfile, "#include \"%s\"\n", lst->files[j] + 6); // Skip "build/"
                } else {
                    fprintf(headerfile, "#include \"../%s\"\n", lst->files[j]);
                }
            }
        }
    }
    va_end(args);

    // Include all build header files
    // for (size_t i = 0; i < build_headers->count; i++) {
	// 	if (!build_headers->files[i]) {
	// 		continue; // Skip null entries
	// 	}
	// 	// Skip the build.h itself to avoid circular inclusion
	// 	if (strcmp(build_headers->files[i], "build/build.h") != 0) {
	// 		if (strstr(build_headers->files[i], "build/") == build_headers->files[i]) {
	// 			// If the file is in the build directory, include it directly
	// 			fprintf(headerfile, "#include \"%s\"\n", build_headers->files[i] + 6); // Skip "build/"
	// 		} else {
	// 			fprintf(headerfile, "#include \"%s\"\n", build_headers->files[i]);
	// 		}
	// 	}
    // }

    // // Include all route header files
    // for (size_t i = 0; i < route_headers->count; i++) {
    //     fprintf(headerfile, "#include \"../%s\"\n", route_headers->files[i]);
    // }

	fprintf(headerfile, "\n\n void auto_routes(int toggle);\n");

    // fclose(headerfile);

    // printf("Generated build.h with %zu build headers and %zu route headers\n",
    //        build_headers->count, route_headers->count);
    return 0;
}


int generate_auto_routes(const char *routes_dir, const char *output_file) {
    file_list_t route_files = {0};

    if (find_files_with_extension(routes_dir, ".c", &route_files) != 0) {
        fprintf(stderr, "Error: Failed to find .c files in '%s'\n", routes_dir);
        cleanup_file_list(&route_files);
        return -1;
    }

    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "Error: Could not create file '%s'\n", output_file);
        cleanup_file_list(&route_files);
        return -1;
    }

    fprintf(file, "#include \"build.h\"\n\n");
    fprintf(file, "void auto_routes(int toggle) {\n");
    fprintf(file, "    if (!toggle) return;\n\n");

    // Genriere add_route-Aufrufe für jede gefundene Datei
   	for (size_t i = 0; i < route_files.count; i++) {
		const char *file_path = route_files.files[i];
		const char *basename = strrchr(file_path, '/');
		basename = basename ? basename + 1 : file_path;
	
		// Entferne die letzten 7 Zeichen (".page.c")
		char route_name[256];
		snprintf(route_name, sizeof(route_name), "%s", basename);
		size_t len = strlen(route_name);
		if (len > 7) {
			route_name[len - 7] = '\0'; // Entferne die letzten 7 Zeichen
		}
	
		fprintf(file, "    cweb_add_route(\"/%s\", %s_page, false);\n", route_name, route_name);
	}

    fprintf(file, "}\n");
    fclose(file);

    // Speicher freigeben
    cleanup_file_list(&route_files);

    printf("Generated auto_routes.c in '%s'\n", output_file);
    return 0;
}

int handle_build_command(int argc, char *argv[]) {
    printf("Starting cweb build process...\n");
    
    static const char *const verbose_aliases[] = {"-v"};
    static const char *const help_aliases[] = {"-h", "-help"};

    int verbose = 0;
    for (int i = 1; i < argc; i++) {
        if (matches_alias(argv[i], "--verbose", verbose_aliases, sizeof(verbose_aliases) / sizeof(verbose_aliases[0]))) {
            verbose = 1;
            continue;
        }

        if (matches_alias(argv[i], "--help", help_aliases, sizeof(help_aliases) / sizeof(help_aliases[0]))) {
            printf("Usage: cweb build [options]\n");
            printf("Options:\n");
            printf("  -v, --verbose    Enable verbose output\n");
            printf("  -h, --help       Show this help message\n");
            return 0;
        }
    }
    
    if (create_directory_if_not_exists("build") != 0) {
        return 1;
    }
    
    // Find all .cweb files in app/templates
    file_list_t cweb_files = {0};
    if (verbose) printf("Searching for .cweb files in app/templates...\n");
    
    if (find_files_with_extension("app/templates", ".cweb", &cweb_files) != 0) {
        cleanup_file_list(&cweb_files);
        return 1;
    }
    
    printf("Found %zu .cweb files\n", cweb_files.count);
    
    // Compile all .cweb files
    for (size_t i = 0; i < cweb_files.count; i++) {
        if (cweb_compile(cweb_files.files[i]) != 0) {
            fprintf(stderr, "Error: Failed to compile %s\n", cweb_files.files[i]);
            cleanup_file_list(&cweb_files);
            return 1;
        }
    }

	if (generate_auto_routes("app/routes", "build/auto_routes.c") != 0) {
        fprintf(stderr, "Failed to generate auto_routes.c\n");
        return 1;
    }
    
    file_list_t build_files = {0};
	file_list_t build_headers = {0};
    if (verbose) printf("Scanning build/ directory for .c files...\n");
    
	if (find_files_with_extension("build", ".c", &build_files) != 0 ||
	find_files_with_extension("build", ".h", &build_headers) != 0) {
		cleanup_file_list(&build_files);
		cleanup_file_list(&build_headers);
		return 1;
	}
    
    file_list_t route_files = {0};
	file_list_t route_headers = {0};
    file_list_t plugin_files = {0};
    file_list_t plugin_headers = {0};
    if (verbose) printf("Scanning app/routes for .c files...\n");
    
    if (find_files_with_extension("app/routes", ".c", &route_files) != 0 ||
        find_files_with_extension("app/routes", ".h", &route_headers) != 0) {
        cleanup_file_list(&route_files);
        cleanup_file_list(&route_headers);
        return 1;
    }

    if (find_files_with_extension("app/plugins", ".c", &plugin_files) != 0 ||
        find_files_with_extension("app/plugins", ".h", &plugin_headers) != 0) {
        cleanup_file_list(&plugin_files);
        cleanup_file_list(&plugin_headers);
        return 1;
    }
    
    file_list_t all_sources = {0};
    if (combine_file_lists(&all_sources, 3, &build_files, &route_files, &plugin_files) != 0) {
        cleanup_file_list(&build_files);
        cleanup_file_list(&route_files);
        cleanup_file_list(&plugin_files);
        return 1;
    }

    if (write_sources_conf(&all_sources) != 0) {
		printf("Found\n");
        cleanup_file_list(&build_files);
        cleanup_file_list(&route_files);
        cleanup_file_list(&all_sources);
        return 1;
    }

    if (write_build_h(3, &build_headers, &route_headers, &plugin_headers) != 0) {
        cleanup_file_list(&build_headers);
        cleanup_file_list(&route_headers);
        cleanup_file_list(&plugin_headers);
        return 1;
    }

    printf("Build completed successfully!\n");
    printf("- Found %zu .c files in build/\n", build_files.count);
    printf("- Found %zu .c files in app/routes\n", route_files.count);
    printf("- Found %zu .h files in build/\n", build_headers.count);
    printf("- Found %zu .h files in app/routes\n", route_headers.count);
    printf("- Found %zu .c files in app/plugins\n", plugin_files.count);
    printf("- Found %zu .h files in app/plugins\n", plugin_headers.count);

    cleanup_file_list(&build_files);
    cleanup_file_list(&route_files);
    cleanup_file_list(&build_headers);
    cleanup_file_list(&route_headers);
    cleanup_file_list(&plugin_files);
    cleanup_file_list(&all_sources);

    return 0;
}
