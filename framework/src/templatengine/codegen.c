#include "codegen_internal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool cweb_generate_header(const char *template_content, const char *header_filename) {
    FILE *header_file = fopen(header_filename, "w");
    if (!header_file) {
        fprintf(stderr, "Error: Cannot create header file %s\n", header_filename);
        return false;
    }

    char guard_name[256];
    const char *base_name = strrchr(header_filename, '/');
    base_name = base_name ? base_name + 1 : header_filename;
    snprintf(guard_name, sizeof(guard_name), "_%s_H", base_name);

    for (int i = 0; guard_name[i]; i++) {
        if (guard_name[i] == '.') guard_name[i] = '_';
        guard_name[i] = toupper(guard_name[i]);
    }

    fprintf(header_file, "#ifndef %s\n", guard_name);
    fprintf(header_file, "#define %s\n\n", guard_name);

    fprintf(header_file, "#include <stdio.h>\n");
    fprintf(header_file, "#include <stdlib.h>\n");
    fprintf(header_file, "#include <string.h>\n");
    fprintf(header_file, "#include <time.h>\n");
    fprintf(header_file, "#include <stdbool.h>\n\n");

    template_signature_t signature = (template_signature_t){0};
    bool has_signature = extract_template_signature(template_content, &signature);

    if (signature.header_content_start && signature.header_content_length > 0) {
        fwrite(signature.header_content_start, 1, signature.header_content_length, header_file);
        if (signature.header_content_start[signature.header_content_length - 1] != '\n') {
            fputc('\n', header_file);
        }
    }

    bool has_external_css = false;
    if (has_signature) {
        const char *body_start = template_body_start(&signature, template_content);
        lexer_t *lexer = cweb_lexer_create(body_start);
        if (lexer) {
            parser_t *parser = cweb_parser_create(lexer);
            if (parser) {
                ast_node_t *root = cweb_parser_parse(parser);
                if (root) {
                    has_external_css = template_has_external_css(root);
                }
                cweb_parser_destroy(parser);
            }
            cweb_lexer_destroy(lexer);
        }
    }

    if (has_signature && has_external_css && !header_contains_css_declaration(&signature)) {
        fprintf(header_file, "char* %s_css(%s);\n", signature.func_name, signature.param_signature);
    }

    fprintf(header_file, "\n#endif // %s\n", guard_name);
    fclose(header_file);

    if (!has_signature) {
        fprintf(stderr, "Warning: Could not determine template function signature for %s\n", header_filename);
    }

    return true;
}

bool cweb_generate_c_file(const char *template_content, const char *c_filename, const char *header_filename) {
    FILE *c_file = fopen(c_filename, "w");
    if (!c_file) {
        fprintf(stderr, "Error: Cannot create C file %s\n", c_filename);
        return false;
    }

    template_signature_t signature = {0};
    if (!extract_template_signature(template_content, &signature)) {
        fprintf(stderr, "Error: Could not determine template function signature for %s\n", c_filename);
        fclose(c_file);
        return false;
    }

    const char *param_signature = signature.param_signature;
    if (!param_signature) {
        param_signature = "";
    }

    const char *content_start = template_body_start(&signature, template_content);

    lexer_t *lexer = cweb_lexer_create(content_start);
    if (!lexer) {
        fclose(c_file);
        return false;
    }

    parser_t *parser = cweb_parser_create(lexer);
    if (!parser) {
        cweb_lexer_destroy(lexer);
        fclose(c_file);
        return false;
    }
    
    ast_node_t *root = cweb_parser_parse(parser);
    if (!root) {
        cweb_parser_destroy(parser);
        cweb_lexer_destroy(lexer);
        fclose(c_file);
        return false;
    }
    
    bool has_external_css = template_has_external_css(root);

    char header_include[256];
    const char *last_slash = strrchr(header_filename, '/');
    const char *header_base = last_slash ? last_slash + 1 : header_filename;
    snprintf(header_include, sizeof(header_include), "#include \"%s\"\n", header_base);
    fprintf(c_file, "%s", header_include);
    fprintf(c_file, "#include <cweb/template.h>\n\n");

    fprintf(c_file, "char* %s(%s) {\n", signature.func_name, param_signature);
    fprintf(c_file, "    cweb_output_init();\n\n");

    generate_html_node(c_file, root);

    fprintf(c_file, "\n    return cweb_output_get();\n");
    fprintf(c_file, "}\n\n");

    if (has_external_css) {
        fprintf(c_file, "char* %s_css(%s) {\n", signature.func_name, param_signature);
        fprintf(c_file, "    cweb_output_init();\n");
        fprintf(c_file, "\n");
        emit_css_node(c_file, root, false);
        fprintf(c_file, "\n    return cweb_output_get();\n");
        fprintf(c_file, "}\n");
    }

    cweb_parser_destroy(parser);
    cweb_lexer_destroy(lexer);
    fclose(c_file);
    return true;
}

// Main compilation function
bool cweb_compile_template(const char *template_file, const char *output_base) {
    FILE *file = fopen(template_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open template file %s\n", template_file);
        return false;
    }
    
    // Read entire file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(file_size + 1);
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return false;
    }
    
    size_t read_bytes = fread(content, 1, (size_t)file_size, file);
    if (read_bytes != (size_t)file_size) {
        free(content);
        fclose(file);
        fprintf(stderr, "Error: Failed to read template %s\n", template_file);
        return false;
    }
    content[file_size] = '\0';
    fclose(file);
    
    // Generate output filenames
    char c_filename[256];
    char h_filename[256];
    
    snprintf(c_filename, sizeof(c_filename), "%s.c", output_base);
    snprintf(h_filename, sizeof(h_filename), "%s.h", output_base);
    
    // Generate files
    bool success = cweb_generate_header(content, h_filename) && 
                   cweb_generate_c_file(content, c_filename, h_filename);
    
    if (success) {
        printf("Generated: %s and %s\n", c_filename, h_filename);
    }
    
    free(content);
    return success;
}
