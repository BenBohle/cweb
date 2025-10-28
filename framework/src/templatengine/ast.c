#include <cweb/codegen.h>

ast_node_t* cweb_ast_node_create(ast_node_type_t type) {
    ast_node_t *node = calloc(1, sizeof(ast_node_t));
    if (!node) return NULL;
    
    node->type = type;
    node->content = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->line = 0;
    node->column = 0;
    
    return node;
}

void cweb_ast_node_destroy(ast_node_t *node) {
    if (!node) return;
    
    if (node->content) {
        free(node->content);
    }
    
    if (node->children) {
        for (size_t i = 0; i < node->child_count; i++) {
            cweb_ast_node_destroy(node->children[i]);
        }
        free(node->children);
    }
    
    free(node);
}

bool cweb_ast_node_add_child(ast_node_t *parent, ast_node_t *child) {
    if (!parent || !child) return false;
    
    ast_node_t **new_children = realloc(parent->children, 
                                       (parent->child_count + 1) * sizeof(ast_node_t*));
    if (!new_children) return false;
    
    parent->children = new_children;
    parent->children[parent->child_count] = child;
    parent->child_count++;
    
    return true;
}
