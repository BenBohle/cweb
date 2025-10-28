#ifndef CWEB_CODEGEN_INTERNAL_H
#define CWEB_CODEGEN_INTERNAL_H

#include <cweb/codegen.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool inline_style;
    char attributes[256];
} cstyle_tag_info_t;

typedef struct {
    char func_name[128];
    char param_signature[256];
    const char *header_content_start;
    const char *header_end;
    size_t header_content_length;
} template_signature_t;

bool extract_template_signature(const char *template_content, template_signature_t *signature);
bool header_contains_css_declaration(const template_signature_t *signature);
const char* template_body_start(const template_signature_t *signature, const char *template_content);

void parse_cstyle_tag_info(const char *tag, cstyle_tag_info_t *info);
bool node_contains_css_content(const ast_node_t *node, bool inside_css_block);
bool template_has_external_css(const ast_node_t *node);
void emit_css_node(FILE *c_file, ast_node_t *node, bool inside_css_block);
void emit_css_children(FILE *c_file, ast_node_t *node);

void remove_type_suffix(char *var_expr, const char *suffix);
void generate_variable_output(FILE *c_file, char *var_expr, bool escape_html);
void write_escaped_string(FILE *file, const char *str);
void parse_cloop_params(const char *cloop_tag, char *var_name, char *array_expr, char *count_expr);
void parse_cif_condition(const char *cif_tag, char *condition);
void generate_html_node(FILE *c_file, ast_node_t *node);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_CODEGEN_INTERNAL_H */
