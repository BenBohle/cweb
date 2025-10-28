#include <cweb/codegen.h>
#include <cweb/leak_detector.h>

parser_t* cweb_parser_create(lexer_t *lexer) {
    if (!lexer) return NULL;
    
    parser_t *parser = calloc(1, sizeof(parser_t));
    if (!parser) return NULL;
    cweb_leak_tracker_record("parser", parser, sizeof(*parser), true);

    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    parser->root = cweb_ast_node_create(AST_ROOT);
    
    if (!parser->root) {
        cweb_leak_tracker_record("parser", parser, sizeof(*parser), false);
        free(parser);
        return NULL;
    }
    
    return parser;
}

void cweb_parser_destroy(parser_t *parser) {
    if (parser) {
        if (parser->current_token.value) {
            cweb_leak_tracker_record("parser.token.value", parser->current_token.value, strlen(parser->current_token.value) + 1, false);
            free(parser->current_token.value);
        }
        if (parser->root) {
            cweb_ast_node_destroy(parser->root);
        }
        cweb_leak_tracker_record("parser", parser, sizeof(*parser), false);
        free(parser);
    }
}

static void parser_advance(parser_t *parser) {
    if (parser->current_token.value) {
        cweb_leak_tracker_record("parser.token.value", parser->current_token.value, strlen(parser->current_token.value) + 1, false);
        free(parser->current_token.value);
        parser->current_token.value = NULL;
    }
    parser->current_token = lexer_next_token(parser->lexer);
}

static ast_node_t* parser_parse_element(parser_t *parser);

static ast_node_t* parser_parse_cstyle(parser_t *parser) {
    ast_node_t *node = cweb_ast_node_create(AST_CSTYLE);
    if (!node) return NULL;

    node->content = strdup(parser->current_token.value);
    if (node->content) cweb_leak_tracker_record("ast.node.content", node->content, strlen(node->content) + 1, true);
    node->line = parser->current_token.line;
    node->column = parser->current_token.column;

    parser_advance(parser); // Skip <cstyle>

    while (parser->current_token.type != TOKEN_EOF &&
           parser->current_token.type != TOKEN_CENDSTYLE) {
        ast_node_t *child = parser_parse_element(parser);
        if (child) {
            cweb_ast_node_add_child(node, child);
        }
    }

    if (parser->current_token.type == TOKEN_CENDSTYLE) {
        parser_advance(parser);
    }

    return node;
}

static ast_node_t* parser_parse_cloop(parser_t *parser) {
    ast_node_t *node = cweb_ast_node_create(AST_CLOOP);
    if (!node) return NULL;
    
    // Extract loop parameters from <cloop ...>
    node->content = strdup(parser->current_token.value);
    if (node->content) cweb_leak_tracker_record("ast.node.content", node->content, strlen(node->content) + 1, true);
    node->line = parser->current_token.line;
    node->column = parser->current_token.column;
    
    parser_advance(parser); // Skip <cloop>
    
    // Parse loop body
    while (parser->current_token.type != TOKEN_EOF && 
           parser->current_token.type != TOKEN_CENDLOOP) {
        ast_node_t *child = parser_parse_element(parser);
        if (child) {
            cweb_ast_node_add_child(node, child);
        }
    }
    
    // Skip </cloop>
    if (parser->current_token.type == TOKEN_CENDLOOP) {
        parser_advance(parser);
    }
    
    return node;
}

static ast_node_t* parser_parse_cif(parser_t *parser) {
    ast_node_t *node = cweb_ast_node_create(AST_CIF);
    if (!node) return NULL;
    
    // Extract condition from <cif ...>
    node->content = strdup(parser->current_token.value);
    if (node->content) cweb_leak_tracker_record("ast.node.content", node->content, strlen(node->content) + 1, true);
    node->line = parser->current_token.line;
    node->column = parser->current_token.column;
    
    parser_advance(parser); // Skip <cif>
    
    // Parse if body
    while (parser->current_token.type != TOKEN_EOF && 
           parser->current_token.type != TOKEN_CELSE &&
           parser->current_token.type != TOKEN_CENDIF) {
        ast_node_t *child = parser_parse_element(parser);
        if (child) {
            cweb_ast_node_add_child(node, child);
        }
    }
    
    // Handle else clause
    if (parser->current_token.type == TOKEN_CELSE) {
        ast_node_t *else_node = cweb_ast_node_create(AST_CELSE);
        if (else_node) {
            parser_advance(parser); // Skip <celse>
            
            // Parse else body
            while (parser->current_token.type != TOKEN_EOF && 
                   parser->current_token.type != TOKEN_CENDIF) {
                ast_node_t *child = parser_parse_element(parser);
                if (child) {
                    cweb_ast_node_add_child(else_node, child);
                }
            }
            
            cweb_ast_node_add_child(node, else_node);
        }
    }
    
    // Skip </cif>
    if (parser->current_token.type == TOKEN_CENDIF) {
        parser_advance(parser);
    }
    
    return node;
}

static ast_node_t* parser_parse_element(parser_t *parser) {
    ast_node_t *node = NULL;
    
    switch (parser->current_token.type) {
        case TOKEN_HTML: {
            node = cweb_ast_node_create(AST_HTML);
            if (node) {
                node->content = strdup(parser->current_token.value);
                if (node->content) cweb_leak_tracker_record("ast.node.content", node->content, strlen(node->content) + 1, true);
                node->line = parser->current_token.line;
                node->column = parser->current_token.column;
            }
            parser_advance(parser);
            break;
        }
        
        case TOKEN_C_CODE: {
            node = cweb_ast_node_create(AST_C_CODE);
            if (node) {
                node->content = strdup(parser->current_token.value);
                if (node->content) cweb_leak_tracker_record("ast.node.content", node->content, strlen(node->content) + 1, true);
                node->line = parser->current_token.line;
                node->column = parser->current_token.column;
            }
            parser_advance(parser);
            break;
        }
        
        case TOKEN_VARIABLE: {
            node = cweb_ast_node_create(AST_VARIABLE);
            if (node) {
                node->content = strdup(parser->current_token.value);
                if (node->content) cweb_leak_tracker_record("ast.node.content", node->content, strlen(node->content) + 1, true);
                node->line = parser->current_token.line;
                node->column = parser->current_token.column;
            }
            parser_advance(parser);
            break;
        }
        
        case TOKEN_CLOOP: {
            node = parser_parse_cloop(parser);
            break;
        }

        case TOKEN_CIF: {
            node = parser_parse_cif(parser);
            break;
        }

        case TOKEN_CSTYLE: {
            node = parser_parse_cstyle(parser);
            break;
        }

        default:
            // Skip unknown tokens
            parser_advance(parser);
            break;
    }
    
    return node;
}

ast_node_t* cweb_parser_parse(parser_t *parser) {
    if (!parser || !parser->root) return NULL;
    
    while (parser->current_token.type != TOKEN_EOF) {
        ast_node_t *element = parser_parse_element(parser);
        if (element) {
            cweb_ast_node_add_child(parser->root, element);
        }
    }
    
    return parser->root;
}
