#include <cweb/codegen.h>
#include <cweb/leak_detector.h>
#include <ctype.h>
#include <string.h>

lexer_t* cweb_lexer_create(const char *input) {
    if (!input) return NULL;
    
    lexer_t *lexer = calloc(1, sizeof(lexer_t));
    if (!lexer) return NULL;
    cweb_leak_tracker_record("lexer", lexer, sizeof(*lexer), true);
    
    lexer->input = input;
    lexer->length = strlen(input);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    
    return lexer;
}

void cweb_lexer_destroy(lexer_t *lexer) {
    if (lexer) {
        cweb_leak_tracker_record("lexer", lexer, sizeof(*lexer), false);
        free(lexer);
    }
}

static char lexer_peek(lexer_t *lexer) {
    if (lexer->position >= lexer->length) {
        return '\0';
    }
    return lexer->input[lexer->position];
}

static char lexer_peek_ahead(lexer_t *lexer, size_t offset) {
    if (lexer->position + offset >= lexer->length) {
        return '\0';
    }
    return lexer->input[lexer->position + offset];
}

static char lexer_peek_back(lexer_t *lexer, size_t offset) {
	if (lexer->position < offset) {
		return '\0';
	}
	return lexer->input[lexer->position - offset];
}

static char lexer_advance(lexer_t *lexer) {
    if (lexer->position >= lexer->length) {
        return '\0';
    }
    
    char c = lexer->input[lexer->position++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

static token_t lexer_create_token(token_type_t type, const char *value, size_t line, size_t column) {
    token_t token = {0};
    token.type = type;
    token.line = line;
    token.column = column;
    
    if (value) {
        size_t len = strlen(value);
        token.value = malloc(len + 1);
        if (token.value) {
            memcpy(token.value, value, len);
            token.value[len] = '\0';
            cweb_leak_tracker_record("token.value", token.value, len + 1, true);
        }
    }
    
    return token;
}

static bool is_inside_style_tag(lexer_t *lexer) {
    const char *current_pos = lexer->input + lexer->position;
    const char *last_style_like_open = NULL;
    const char *last_style_like_close = NULL;

    for (const char *p = lexer->input; p < current_pos; p++) {
        if (strncmp(p, "<style", 6) == 0 || strncmp(p, "<cstyle", 7) == 0) {
            last_style_like_open = p;
        }
        if (strncmp(p, "</style>", 8) == 0 || strncmp(p, "</cstyle>", 9) == 0) {
            last_style_like_close = p;
        }
    }

    return last_style_like_open && (!last_style_like_close || last_style_like_open > last_style_like_close);
}

static bool is_inside_script_tag(lexer_t *lexer) {
    const char *current_pos = lexer->input + lexer->position;
    const char *last_script_open = NULL;
    const char *last_script_close = NULL;
    
    for (const char *p = lexer->input; p < current_pos; p++) {
        if (strncmp(p, "<script", 7) == 0) {
            last_script_open = p;
        }
        if (strncmp(p, "</script>", 9) == 0) {
            last_script_close = p;
        }
    }
    
    return last_script_open && (!last_script_close || last_script_open > last_script_close);
}

static token_t lexer_read_html(lexer_t *lexer) {
    char *buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) {
        return lexer_create_token(TOKEN_ERROR, "Memory allocation failed", lexer->line, lexer->column);
    }

    size_t pos = 0;
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    
    while (lexer_peek(lexer) != '\0' && pos < MAX_BUFFER_SIZE - 1) {
        char c = lexer_peek(lexer);
        
        // Check for C code block
        if (c == '<' && lexer_peek_ahead(lexer, 1) == '%') {
            break;
        }
        
        // Check for control structures
        if (c == '<') {
            if (strncmp(lexer->input + lexer->position, "<cloop", 6) == 0 ||
                strncmp(lexer->input + lexer->position, "<cif", 4) == 0 ||
                strncmp(lexer->input + lexer->position, "<celse>", 7) == 0 ||
                strncmp(lexer->input + lexer->position, "</cloop>", 8) == 0 ||
                strncmp(lexer->input + lexer->position, "</cif>", 6) == 0 ||
                strncmp(lexer->input + lexer->position, "<cstyle", 7) == 0 ||
                strncmp(lexer->input + lexer->position, "</cstyle>", 9) == 0) {
                break;
            }
        }
        
        // Check for variable interpolation
        if (c == '{' && !is_inside_style_tag(lexer) && !is_inside_script_tag(lexer)) {
            bool looks_like_variable = false;
            size_t lookahead = 1;
            while (lexer_peek_ahead(lexer, lookahead) != '\0' && 
                   lexer_peek_ahead(lexer, lookahead) != '}' && 
                   lookahead < 100) {
                if (lexer_peek_ahead(lexer, lookahead) == '-' && 
                    lexer_peek_ahead(lexer, lookahead + 1) == '>') {
                    looks_like_variable = true;
                    break;
                }
				if (isalnum(lexer_peek_ahead(lexer, lookahead))) {
					// fprintf(stderr, "Looks like a variable: %c\n", lexer_peek_ahead(lexer, lookahead));
					looks_like_variable = true;
				}
                lookahead++;
            }
            
            if (looks_like_variable) {
                break;
            }
        }
        
        buffer[pos++] = lexer_advance(lexer);
    }
    
    buffer[pos] = '\0';
    
    if (pos == 0) {
        free(buffer);
        return lexer_create_token(TOKEN_EOF, NULL, start_line, start_column);
    }
    
    token_t token = lexer_create_token(TOKEN_HTML, buffer, start_line, start_column);
    free(buffer);
    return token;
}

static token_t lexer_read_c_code(lexer_t *lexer) {
    char *buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) {
        return lexer_create_token(TOKEN_ERROR, "Memory allocation failed", lexer->line, lexer->column);
    }
    
    size_t pos = 0;
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    
    // Skip <%
    lexer_advance(lexer); // <
    lexer_advance(lexer); // %
    
    while (lexer_peek(lexer) != '\0' && pos < MAX_BUFFER_SIZE - 1) {
        char c = lexer_peek(lexer);
        
        // Check for end of C code block
        if (c == '%' && lexer_peek_ahead(lexer, 1) == '>') {
            lexer_advance(lexer); // %
            lexer_advance(lexer); // >
            break;
        }
        
        buffer[pos++] = lexer_advance(lexer);
    }
    
    buffer[pos] = '\0';
    
    // Validate C code for security
    if (!cweb_sanitize_c_code(buffer)) {
        free(buffer);
        return lexer_create_token(TOKEN_ERROR, "Invalid C code detected", start_line, start_column);
    }
    
    token_t token = lexer_create_token(TOKEN_C_CODE, buffer, start_line, start_column);
    free(buffer);
    return token;
}

static token_t lexer_read_variable(lexer_t *lexer) {
    char *buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) {
        return lexer_create_token(TOKEN_ERROR, "Memory allocation failed", lexer->line, lexer->column);
    }
    
    size_t pos = 0;
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    
    // Skip {
    lexer_advance(lexer);
    
    while (lexer_peek(lexer) != '\0' && lexer_peek(lexer) != '}' && pos < MAX_BUFFER_SIZE - 1) {
        buffer[pos++] = lexer_advance(lexer);
    }
    
    if (lexer_peek(lexer) == '}') {
        lexer_advance(lexer); // Skip }
    }
    
    buffer[pos] = '\0';
    
    // Trim whitespace
    char *start = buffer;
    while (isspace(*start)) start++;
    
    char *end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) *end-- = '\0';
    
    token_t token = lexer_create_token(TOKEN_VARIABLE, start, start_line, start_column);
    free(buffer);
    return token;
}

int is_inside_tag_condition(lexer_t *lexer) {
	if (lexer_peek(lexer) == '\0') {
        return -1;
    }

    if (lexer_peek(lexer) == '>' && lexer_peek_back(lexer, 1) == '-') {
        return 1;
    } 
	else if (lexer_peek(lexer) == '>' && lexer_peek_back(lexer, 1) != '-')
	{
		return 0;
	}
    
    return 1;
}

static token_t lexer_read_control_tag(lexer_t *lexer) {
    char *buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) {
        return lexer_create_token(TOKEN_ERROR, "Memory allocation failed", lexer->line, lexer->column);
    }
    
    size_t pos = 0;
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    
    // Read the entire tag
    while (lexer_peek(lexer) != '\0' && is_inside_tag_condition(lexer) > 0 && pos < MAX_BUFFER_SIZE - 1) {
        buffer[pos++] = lexer_advance(lexer);
    }
    
    if (lexer_peek(lexer) == '>') {
        buffer[pos++] = lexer_advance(lexer);
    }
    
    buffer[pos] = '\0';
    
    // Determine token type
    token_type_t type = TOKEN_ERROR;
    if (strncmp(buffer, "<cloop", 6) == 0) {
        type = TOKEN_CLOOP;
    } else if (strncmp(buffer, "<cif", 4) == 0) {
        type = TOKEN_CIF;
    } else if (strncmp(buffer, "<celse>", 7) == 0) {
        type = TOKEN_CELSE;
    } else if (strncmp(buffer, "</cloop>", 8) == 0) {
        type = TOKEN_CENDLOOP;
    } else if (strncmp(buffer, "</cif>", 6) == 0) {
        type = TOKEN_CENDIF;
    } else if (strncmp(buffer, "<cstyle", 7) == 0) {
        type = TOKEN_CSTYLE;
    } else if (strncmp(buffer, "</cstyle>", 9) == 0) {
        type = TOKEN_CENDSTYLE;
    }
    
    token_t token = lexer_create_token(type, buffer, start_line, start_column);
    free(buffer);
    return token;
}

token_t lexer_next_token(lexer_t *lexer) {
    if (!lexer || lexer->position >= lexer->length) {
        return lexer_create_token(TOKEN_EOF, NULL, lexer->line, lexer->column);
    }
    
    char c = lexer_peek(lexer);
    
    // Check for C code block
    if (c == '<' && lexer_peek_ahead(lexer, 1) == '%') {
        return lexer_read_c_code(lexer);
    }
    
    // Check for control structures
    if (c == '<') {
        if (strncmp(lexer->input + lexer->position, "<cloop", 6) == 0 ||
            strncmp(lexer->input + lexer->position, "<cif", 4) == 0 ||
            strncmp(lexer->input + lexer->position, "<celse>", 7) == 0 ||
            strncmp(lexer->input + lexer->position, "</cloop>", 8) == 0 ||
            strncmp(lexer->input + lexer->position, "</cif>", 6) == 0 ||
            strncmp(lexer->input + lexer->position, "<cstyle", 7) == 0 ||
            strncmp(lexer->input + lexer->position, "</cstyle>", 9) == 0) {
            return lexer_read_control_tag(lexer);
        }
    }
    
    // Check for variable interpolation
    if (c == '{' && !is_inside_style_tag(lexer) && !is_inside_script_tag(lexer)) {
        bool looks_like_variable = false;
        size_t lookahead = 1;
        while (lexer_peek_ahead(lexer, lookahead) != '\0' && 
               lexer_peek_ahead(lexer, lookahead) != '}' && 
               lookahead < 100) {
            if (lexer_peek_ahead(lexer, lookahead) == '-' && 
                lexer_peek_ahead(lexer, lookahead + 1) == '>') {
                looks_like_variable = true;
                break;
            }
			if (isalnum(lexer_peek_ahead(lexer, lookahead))) {
				// fprintf(stderr, "Looks like a variable: %c\n", lexer_peek_ahead(lexer, lookahead));
				looks_like_variable = true;
			}
            lookahead++;
        }
        
        if (looks_like_variable) {
            return lexer_read_variable(lexer);
        }
    }
    
    // Default to HTML content
    return lexer_read_html(lexer);
}
