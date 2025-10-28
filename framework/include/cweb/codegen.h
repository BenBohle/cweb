#ifndef CWEB_CODEGEN_H
#define CWEB_CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <stddef.h>
#include <cweb/autofree.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BUFFER_SIZE 8192
#define MAX_TAG_NAME 64
#define MAX_ATTRIBUTE_NAME 64
#define MAX_ATTRIBUTE_VALUE 256

// Token types for lexer
typedef enum {
    TOKEN_HTML,
    TOKEN_C_CODE,
    TOKEN_VARIABLE,
    TOKEN_CLOOP,
    TOKEN_CIF,
    TOKEN_CELSE,
    TOKEN_CENDLOOP,
    TOKEN_CENDIF,
    TOKEN_CSTYLE,
    TOKEN_CENDSTYLE,
    TOKEN_EOF,
    TOKEN_ERROR
} token_type_t;

// Token
typedef struct {
    token_type_t type;
    char *value;
    size_t line;
    size_t column;
} token_t;

// AST node types
typedef enum {
    AST_HTML,
    AST_C_CODE,
    AST_VARIABLE,
    AST_CLOOP,
    AST_CIF,
    AST_CELSE,
    AST_CSTYLE,
    AST_ROOT
} ast_node_type_t;

// Forward declaration for AST node
typedef struct ast_node ast_node_t;

// AST node structure
struct ast_node {
    ast_node_type_t type;
    char *content;
    ast_node_t **children;
    size_t child_count;
    size_t line;
    size_t column;
};

// Lexer structure
typedef struct {
    const char *input;
    size_t position;
    size_t line;
    size_t column;
    size_t length;
} lexer_t;

// Parser structure
typedef struct {
    lexer_t *lexer;
    token_t current_token;
    ast_node_t *root;
} parser_t;

// Function declarations
lexer_t* cweb_lexer_create(const char *input);
void cweb_lexer_destroy(lexer_t *lexer);
token_t lexer_next_token(lexer_t *lexer);

parser_t* cweb_parser_create(lexer_t *lexer);
void cweb_parser_destroy(parser_t *parser);
ast_node_t* cweb_parser_parse(parser_t *parser);

// AST functions
ast_node_t* cweb_ast_node_create(ast_node_type_t type);
void cweb_ast_node_destroy(ast_node_t *node);
bool cweb_ast_node_add_child(ast_node_t *parent, ast_node_t *child);

// Template compilation functions
bool cweb_compile_template(const char *template_file, const char *output_base);
bool cweb_generate_header(const char *template_content, const char *header_filename);
bool cweb_generate_c_file(const char *template_content, const char *c_filename, const char *header_filename);

// Security functions
char* cweb_escape_html(const char *input);
bool cweb_validate_tag_name(const char *tag);
bool cweb_sanitize_c_code(const char *code);

#ifdef __cplusplus
}
#endif

#endif /* CWEB_CODEGEN_H */
