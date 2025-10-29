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

void parse_cstyle_tag_info(const char *tag, cstyle_tag_info_t *info) {
    if (!info) {
        return;
    }

    info->inline_style = false;
    info->attributes[0] = '\0';

    if (!tag) {
        return;
    }

    const char *start = strstr(tag, "<cstyle");
    if (!start) {
        return;
    }

    start += strlen("<cstyle");
    const char *end = strchr(start, '>');
    if (!end) {
        return;
    }

    size_t raw_len = (size_t)(end - start);
    if (raw_len == 0) {
        return;
    }

    char buffer[512];
    if (raw_len >= sizeof(buffer)) {
        raw_len = sizeof(buffer) - 1;
    }

    memcpy(buffer, start, raw_len);
    buffer[raw_len] = '\0';

    char *cursor = buffer;
    char *out = info->attributes;
    size_t out_len = 0;
    bool first_attr = true;

    while (*cursor) {
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }

        if (*cursor == '\0') {
            break;
        }

        char *name_start = cursor;
        while (*cursor && !isspace((unsigned char)*cursor) && *cursor != '=') {
            cursor++;
        }

        size_t name_len = (size_t)(cursor - name_start);
        if (name_len == 0) {
            break;
        }

        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }

        bool has_value = false;
        if (*cursor == '=') {
            has_value = true;
            cursor++;
            while (isspace((unsigned char)*cursor)) {
                cursor++;
            }

            if (*cursor == '"' || *cursor == '\'') {
                char quote = *cursor++;
                while (*cursor && *cursor != quote) {
                    cursor++;
                }
                if (*cursor == quote) {
                    cursor++;
                }
            } else {
                while (*cursor && !isspace((unsigned char)*cursor)) {
                    cursor++;
                }
            }
        }

        const char *attr_end = cursor;
        while (attr_end > name_start && isspace((unsigned char)*(attr_end - 1))) {
            attr_end--;
        }
        if (!has_value) {
            while (attr_end > name_start && *(attr_end - 1) == '/') {
                attr_end--;
            }
        }

        bool is_setinline = (name_len == strlen("setinline") && strncmp(name_start, "setinline", name_len) == 0);
        if (is_setinline) {
            info->inline_style = true;
            continue;
        }

        size_t copy_len = (size_t)(attr_end - name_start);
        if (copy_len == 0) {
            continue;
        }

        if (!first_attr) {
            if (out_len + 1 < sizeof(info->attributes)) {
                out[out_len++] = ' ';
            } else {
                break;
            }
        }

        if (copy_len >= sizeof(info->attributes) - out_len) {
            copy_len = sizeof(info->attributes) - out_len - 1;
        }

        memcpy(out + out_len, name_start, copy_len);
        out_len += copy_len;
        out[out_len] = '\0';
        first_attr = false;
    }
}

bool node_contains_css_content(const ast_node_t *node, bool inside_css_block) {
    if (!node) {
        return false;
    }

    switch (node->type) {
        case AST_HTML:
        case AST_VARIABLE:
        case AST_C_CODE:
            return inside_css_block && node->content && node->content[0] != '\0';

        case AST_CSTYLE: {
            cstyle_tag_info_t info;
            parse_cstyle_tag_info(node->content, &info);
            if (info.inline_style && !inside_css_block) {
                return false;
            }

            bool child_inside = info.inline_style ? inside_css_block : true;
            bool has_content = false;
            for (size_t i = 0; i < node->child_count; ++i) {
                if (node_contains_css_content(node->children[i], child_inside)) {
                    has_content = true;
                    break;
                }
            }
            return has_content;
        }

        case AST_CLOOP:
        case AST_CIF:
        case AST_CELSE:
        case AST_ROOT:
            for (size_t i = 0; i < node->child_count; ++i) {
                if (node_contains_css_content(node->children[i], inside_css_block)) {
                    return true;
                }
            }
            return false;

        default:
            return false;
    }
}

bool template_has_external_css(const ast_node_t *node) {
    if (!node) {
        return false;
    }

    if (node->type == AST_CSTYLE) {
        cstyle_tag_info_t info;
        parse_cstyle_tag_info(node->content, &info);
        if (!info.inline_style && node_contains_css_content(node, false)) {
            return true;
        }
    }

    for (size_t i = 0; i < node->child_count; ++i) {
        if (template_has_external_css(node->children[i])) {
            return true;
        }
    }

    return false;
}

void emit_css_node(FILE *c_file, ast_node_t *node, bool inside_css_block) {
    if (!node) {
        return;
    }

    switch (node->type) {
        case AST_HTML:
            if (inside_css_block && node->content && node->content[0] != '\0') {
                fprintf(c_file, "    cweb_output_raw(");
                write_escaped_string(c_file, node->content);
                fprintf(c_file, ");\n");
            }
            break;

        case AST_C_CODE:
            if (inside_css_block && node->content && node->content[0] != '\0') {
                fprintf(c_file, "%s\n", node->content);
            }
            break;

        case AST_VARIABLE:
            if (inside_css_block && node->content && node->content[0] != '\0') {
                generate_variable_output(c_file, node->content, false);
            }
            break;

        case AST_CLOOP: {
            bool body_has_css = false;
            for (size_t i = 0; i < node->child_count; ++i) {
                if (node_contains_css_content(node->children[i], inside_css_block)) {
                    body_has_css = true;
                    break;
                }
            }

            if (!body_has_css) {
                break;
            }

            char var_name[64] = "item";
            char array_expr[256] = "";
            char count_expr[256] = "";
            parse_cloop_params(node->content, var_name, array_expr, count_expr);

            fprintf(c_file, "    // Loop: %s\n", node->content);
            fprintf(c_file, "    for (size_t i = 0; i < %s; i++) {\n", count_expr);
            if (strlen(array_expr) > 0) {
                fprintf(c_file, "        const char *%s = %s[i];\n", var_name, array_expr);
            }

            for (size_t i = 0; i < node->child_count; ++i) {
                emit_css_node(c_file, node->children[i], inside_css_block);
            }

            fprintf(c_file, "    }\n");
            break;
        }

        case AST_CIF: {
            bool then_has_css = false;
            bool else_has_css = false;
            ast_node_t *else_node = NULL;

            for (size_t i = 0; i < node->child_count; ++i) {
                ast_node_t *child = node->children[i];
                if (child->type == AST_CELSE) {
                    else_node = child;
                    else_has_css = node_contains_css_content(child, inside_css_block);
                } else if (!then_has_css && node_contains_css_content(child, inside_css_block)) {
                    then_has_css = true;
                }
            }

            if (!then_has_css && !else_has_css) {
                break;
            }

            char condition[256] = "";
            parse_cif_condition(node->content, condition);

            if (strlen(condition) == 0) {
                fprintf(c_file, "    // Warning: Empty condition in cif tag\n");
                fprintf(c_file, "    // Tag content: %s\n", node->content ? node->content : "NULL");
                fprintf(c_file, "    if (false) {\n");
            } else {
                fprintf(c_file, "    // Conditional: %s\n", condition);
                fprintf(c_file, "    if (%s) {\n", condition);
            }

            for (size_t i = 0; i < node->child_count; ++i) {
                ast_node_t *child = node->children[i];
                if (child == else_node) {
                    continue;
                }
                emit_css_node(c_file, child, inside_css_block);
            }

            if (else_node && else_has_css) {
                fprintf(c_file, "    } else {\n");
                for (size_t j = 0; j < else_node->child_count; ++j) {
                    emit_css_node(c_file, else_node->children[j], inside_css_block);
                }
            }

            fprintf(c_file, "    }\n");
            break;
        }

        case AST_CSTYLE: {
            cstyle_tag_info_t info;
            parse_cstyle_tag_info(node->content, &info);
            if (info.inline_style) {
                if (inside_css_block) {
                    for (size_t i = 0; i < node->child_count; ++i) {
                        emit_css_node(c_file, node->children[i], true);
                    }
                }
            } else {
                for (size_t i = 0; i < node->child_count; ++i) {
                    emit_css_node(c_file, node->children[i], true);
                }
            }
            break;
        }

        case AST_CELSE:
        case AST_ROOT:
            for (size_t i = 0; i < node->child_count; ++i) {
                emit_css_node(c_file, node->children[i], inside_css_block);
            }
            break;

        default:
            break;
    }
}

void emit_css_children(FILE *c_file, ast_node_t *node) {
    for (size_t i = 0; i < node->child_count; ++i) {
        emit_css_node(c_file, node->children[i], true);
    }
}
