// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "codegen_internal.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void remove_type_suffix(char *var_expr, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    size_t var_len = strlen(var_expr);

    // Prüfe, ob die Zeichenkette mit dem Suffix endet
    if (var_len >= suffix_len && strcmp(var_expr + var_len - suffix_len, suffix) == 0) {
        // Entferne das Suffix, indem die Zeichenkette abgeschnitten wird
        var_expr[var_len - suffix_len] = '\0';
    }
}

void generate_variable_output(FILE *c_file, char *var_expr, bool escape_html) {
    const char *output_func = escape_html ? "cweb_output_html" : "cweb_output_raw";

    if (strstr(var_expr, ":int")) {
        remove_type_suffix(var_expr, ":int");
        fprintf(c_file, "    {\n");
        fprintf(c_file, "        char temp_str[32];\n");
    fprintf(c_file, "        snprintf(temp_str, sizeof(temp_str), \"%%d\", %s);\n", var_expr);
        fprintf(c_file, "        %s(temp_str);\n", output_func);
        fprintf(c_file, "    }\n");
    } else if (strstr(var_expr, ":long")) {
        remove_type_suffix(var_expr, ":long");
        fprintf(c_file, "    {\n");
        fprintf(c_file, "        char temp_str[32];\n");
    fprintf(c_file, "        snprintf(temp_str, sizeof(temp_str), \"%%ld\", %s);\n", var_expr);
        fprintf(c_file, "        %s(temp_str);\n", output_func);
        fprintf(c_file, "    }\n");
    } else if (strstr(var_expr, ":size_t")) {
        remove_type_suffix(var_expr, ":size_t");
        fprintf(c_file, "    {\n");
        fprintf(c_file, "        char temp_str[32];\n");
    fprintf(c_file, "        snprintf(temp_str, sizeof(temp_str), \"%%zu\", %s);\n", var_expr);
        fprintf(c_file, "        %s(temp_str);\n", output_func);
        fprintf(c_file, "    }\n");
    } else if (strstr(var_expr, ":float") || strstr(var_expr, ":double")) {
        remove_type_suffix(var_expr, ":float");
        remove_type_suffix(var_expr, ":double");
        fprintf(c_file, "    {\n");
        fprintf(c_file, "        char temp_str[32];\n");
    fprintf(c_file, "        snprintf(temp_str, sizeof(temp_str), \"%%f\", %s);\n", var_expr);
        fprintf(c_file, "        %s(temp_str);\n", output_func);
        fprintf(c_file, "    }\n");
    } else if (strstr(var_expr, ":number")) {
        remove_type_suffix(var_expr, ":number");
        fprintf(c_file, "    {\n");
        fprintf(c_file, "        char temp_str[32];\n");
    fprintf(c_file, "        snprintf(temp_str, sizeof(temp_str), \"%%d\", %s);\n", var_expr);
        fprintf(c_file, "        %s(temp_str);\n", output_func);
        fprintf(c_file, "    }\n");
    } else if (strstr(var_expr, ":bool")) {
        remove_type_suffix(var_expr, ":bool");
        fprintf(c_file, "    %s(%s ? \"true\" : \"false\");\n", output_func, var_expr);
    } else if (strstr(var_expr, ":time")) {
        remove_type_suffix(var_expr, ":time");
        fprintf(c_file, "    {\n");
        fprintf(c_file, "        char time_str[64];\n");
        fprintf(c_file, "        struct tm *tm_info = localtime(&%s);\n", var_expr);
    fprintf(c_file, "        strftime(time_str, sizeof(time_str), \"%%Y-%%m-%%d %%H:%%M:%%S\", tm_info);\n");
        fprintf(c_file, "        %s(time_str);\n", output_func);
        fprintf(c_file, "    }\n");
    } else if (strstr(var_expr, ":string")) {
        remove_type_suffix(var_expr, ":string");
        fprintf(c_file, "    %s(%s);\n", output_func, var_expr);
    } else if (strstr(var_expr, ":html")) {
        remove_type_suffix(var_expr, ":html");
        fprintf(c_file, "    cweb_output_raw(%s);\n", var_expr);
    } else {
        fprintf(c_file, "    if (%s) %s(%s);\n", var_expr, output_func, var_expr);
    }
}

void write_escaped_string(FILE *file, const char *str) {
    if (!str) return;
    
    fprintf(file, "\"");
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':
                fprintf(file, "\\\"");
                break;
            case '\\':
                fprintf(file, "\\\\");
                break;
            case '\n':
                fprintf(file, "\\n");
                break;
            case '\r':
                fprintf(file, "\\r");
                break;
            case '\t':
                fprintf(file, "\\t");
                break;
            default:
                fprintf(file, "%c", *p);
                break;
        }
    }
    fprintf(file, "\"");
}

void parse_cloop_params(const char *cloop_tag, char *var_name, char *array_expr, char *count_expr) {
    // Initialize with defaults
    strcpy(var_name, "item");
    array_expr[0] = '\0';
    count_expr[0] = '\0';
    
    // Parse <cloop var="item" array="data->items" count="data->item_count">
    const char *var_start = strstr(cloop_tag, "var=\"");
    const char *array_start = strstr(cloop_tag, "array=\"");
    const char *count_start = strstr(cloop_tag, "count=\"");
    
    if (var_start) {
        var_start += 5; // Skip var="
        const char *var_end = strchr(var_start, '"');
        if (var_end) {
            size_t len = var_end - var_start;
            strncpy(var_name, var_start, len);
            var_name[len] = '\0';
        }
    }
    
    if (array_start) {
        array_start += 7; // Skip array="
        const char *array_end = strchr(array_start, '"');
        if (array_end) {
            size_t len = array_end - array_start;
            strncpy(array_expr, array_start, len);
            array_expr[len] = '\0';
        }
    }
    
    if (count_start) {
        count_start += 7; // Skip count="
        const char *count_end = strchr(count_start, '"');
        if (count_end) {
            size_t len = count_end - count_start;
            strncpy(count_expr, count_start, len);
            count_expr[len] = '\0';
        }
    }
}

void parse_cif_condition(const char *cif_tag, char *condition) {
    // Parse <cif condition="data->is_admin">
    const char *cond_start = strstr(cif_tag, "condition=\"");
    if (cond_start) {
        cond_start += strlen("condition=\""); // Skip condition="
        
        // Suche nach dem Ende des Attributwerts (schließe korrekt ab)
        const char *cond_end = NULL;
        for (const char *p = cond_start; *p; p++) {
            if (*p == '"' && (p == cond_start || *(p - 1) != '\\')) { // Ignoriere escaped quotes
                cond_end = p;
                break;
            }
        }
        
        if (cond_end) {
            size_t len = cond_end - cond_start;
            strncpy(condition, cond_start, len);
            condition[len] = '\0';
        } else {
            // Handle missing closing quote
            fprintf(stderr, "Error: Missing closing quote for condition in tag: %s\n", cif_tag);
            condition[0] = '\0';
        }
    } else {
        // If no condition attribute found, set empty condition
        fprintf(stderr, "Warning: No condition attribute found in tag: %s\n", cif_tag);
        condition[0] = '\0';
    }
}
