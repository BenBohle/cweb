#include "codegen_internal.h"

#include <stdio.h>
#include <string.h>

void generate_html_node(FILE *c_file, ast_node_t *node) {
    if (!node) {
        return;
    }

    switch (node->type) {
        case AST_HTML:
            if (node->content && node->content[0] != '\0') {
                fprintf(c_file, "    cweb_output_raw(");
                write_escaped_string(c_file, node->content);
                fprintf(c_file, ");\n");
            }
            break;

        case AST_C_CODE:
            if (node->content && node->content[0] != '\0') {
                fprintf(c_file, "%s\n", node->content);
            }
            break;

        case AST_VARIABLE:
            if (node->content && node->content[0] != '\0') {
                generate_variable_output(c_file, node->content, true);
            }
            break;

        case AST_CLOOP: {
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
                generate_html_node(c_file, node->children[i]);
            }

            fprintf(c_file, "    }\n");
            break;
        }

        case AST_CIF: {
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

            ast_node_t *else_node = NULL;
            for (size_t i = 0; i < node->child_count; ++i) {
                if (node->children[i]->type == AST_CELSE) {
                    else_node = node->children[i];
                    continue;
                }
                generate_html_node(c_file, node->children[i]);
            }

            if (else_node) {
                fprintf(c_file, "    } else {\n");
                for (size_t j = 0; j < else_node->child_count; ++j) {
                    generate_html_node(c_file, else_node->children[j]);
                }
            }

            fprintf(c_file, "    }\n");
            break;
        }

        case AST_CSTYLE: {
            cstyle_tag_info_t info;
            parse_cstyle_tag_info(node->content, &info);
            if (info.inline_style) {
                char style_open[512];
                if (info.attributes[0] != '\0') {
                    snprintf(style_open, sizeof(style_open), "<style %s>", info.attributes);
                } else {
                    snprintf(style_open, sizeof(style_open), "<style>");
                }
                fprintf(c_file, "    cweb_output_raw(");
                write_escaped_string(c_file, style_open);
                fprintf(c_file, ");\n");
                emit_css_children(c_file, node);
                fprintf(c_file, "    cweb_output_raw(\"</style>\");\n");
            }
            break;
        }

        case AST_CELSE:
        case AST_ROOT:
            for (size_t i = 0; i < node->child_count; ++i) {
                generate_html_node(c_file, node->children[i]);
            }
            break;

        default:
            break;
    }
}
