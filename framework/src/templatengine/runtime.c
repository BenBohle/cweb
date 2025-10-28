#include <cweb/template.h>
#include <cweb/codegen.h>
#include <strings.h>
#include <cweb/leak_detector.h>

// Global output buffer
cweb_buffer_t g_output_buffer = {0};

void cweb_buffer_init(cweb_buffer_t *buffer) {
    if (!buffer) return;
    
    buffer->capacity = 8096;
    buffer->data = malloc(buffer->capacity);
    buffer->size = 0;
    
    if (buffer->data) {
        buffer->data[0] = '\0';
        cweb_leak_tracker_record("buffer.data 1", buffer->data, strlen(buffer->data), true);
    }
}

void cweb_buffer_append(cweb_buffer_t *buffer, const char *str) {
    if (!buffer || !str) return;
    
    size_t str_len = strlen(str);
    size_t needed = buffer->size + str_len + 1;
    
    // Resize if needed
    if (needed > buffer->capacity) {
        size_t new_capacity = buffer->capacity;
        while (new_capacity < needed) {
            new_capacity *= 2;
        }
        
        char *new_data = realloc(buffer->data, new_capacity);
        if (!new_data) return;
        if (new_data != buffer->data) {
            if (buffer->data) cweb_leak_tracker_record("buffer.data", buffer->data, strlen(buffer->data), false);
            cweb_leak_tracker_record("buffer.data 2", new_data, strlen(new_data), true);
        }
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
    
    // Append string
    strcpy(buffer->data + buffer->size, str);
    buffer->size += str_len;
}

void cweb_buffer_cleanup(cweb_buffer_t *buffer) {
    if (!buffer) return;
    
    if (buffer->data) {
        cweb_leak_tracker_record("buffer.data", buffer->data, strlen(buffer->data), false);
        free(buffer->data);
        buffer->data = NULL;
    }
    buffer->size = 0;
    buffer->capacity = 0;
}

void cweb_output_init(void) {
    cweb_buffer_cleanup(&g_output_buffer);
    cweb_buffer_init(&g_output_buffer);
}

void cweb_output_raw(const char *str) {
	if (!str) return;
	
    cweb_buffer_append(&g_output_buffer, str);
}

void cweb_output_html(const char *str) {
    if (!str) return;
    
    char *escaped = cweb_escape_html(str);
    if (escaped) {
        cweb_leak_tracker_record("escaped", escaped, strlen(escaped) + 1, true);
        cweb_buffer_append(&g_output_buffer, escaped);
        cweb_leak_tracker_record("escaped", escaped, strlen(escaped) + 1, false);
        free(escaped);
    }
}

char* cweb_output_get(void) {
    if (!g_output_buffer.data) return NULL;
    
    // Return a copy of the buffer
    char *result = malloc(g_output_buffer.size + 1);
    if (result) {
        strcpy(result, g_output_buffer.data);
    }
    
    return result;
}

void cweb_output_cleanup(void) {
    cweb_buffer_cleanup(&g_output_buffer);
}

bool cweb_setAssetLink(char **html,
                  const char *logical_name,
                  const char *resolved_path,
                  const char *rel_attr)
{
    if (!html || !*html || !logical_name || !resolved_path) {
        LOG_WARNING("ASSET", "Ungültige Parameter in setAssetLink");
        return false;
    }

    if (!rel_attr) rel_attr = "stylesheet";

    const char *doc = *html;
    size_t doc_len = strlen(doc);

    /* 1. Versuche vorhandenes href="logical_name" zu finden */
    char pattern[256];
    int pn = snprintf(pattern, sizeof(pattern), "href=\"%s\"", logical_name);
    if (pn <= 0 || (size_t)pn >= sizeof(pattern)) {
        LOG_ERROR("ASSET", "Pattern zu lang");
        return false;
    }

    const char *hit = strstr(doc, pattern);

    if (hit) {
        /* Prüfen ob schon auf resolved_path zeigt */
        char new_pattern[256];
        int n2 = snprintf(new_pattern, sizeof(new_pattern), "href=\"%s\"", resolved_path);
        if (n2 > 0 && (size_t)n2 < sizeof(new_pattern)) {
            if (strstr(doc, new_pattern)) {
                // Bereits korrekt
                return true;
            }
        }

        const char *value_start = hit + strlen("href=\"");
        const char *value_end = strchr(value_start, '"');
        if (!value_end) {
            LOG_WARNING("ASSET", "Ungültiges href Attribut");
            return false;
        }

        size_t old_val_len = (size_t)(value_end - value_start);
        size_t new_val_len = strlen(resolved_path);

        size_t new_size = doc_len - old_val_len + new_val_len + 1;
        char *out = (char*)malloc(new_size);
        if (!out) {
            LOG_ERROR("ASSET", "Allocation failed");
            return false;
        }
        cweb_leak_tracker_record("asset.out", out, new_size, true);

        size_t prefix_len = (size_t)(value_start - doc);
        memcpy(out, doc, prefix_len);
        memcpy(out + prefix_len, resolved_path, new_val_len);
        const char *tail_src = value_start + old_val_len;
        size_t tail_len = doc_len - (size_t)(tail_src - doc);
        memcpy(out + prefix_len + new_val_len, tail_src, tail_len);
        out[new_size - 1] = '\0';

        if (*html) cweb_leak_tracker_record("html.old", *html, strlen(*html) + 1, false);
        free(*html);
        *html = out;
        return true;
    }

    /* 2. Kein vorhandenes Tag für logical_name -> neues <link> einfügen */
    const char *head_close = strcasestr(doc, "</head>");
    char link_tag[512];
    int ln = snprintf(link_tag, sizeof(link_tag),
                      "<link rel=\"%s\" href=\"%s\" />\n",
                      rel_attr, resolved_path);
    if (ln <= 0 || (size_t)ln >= sizeof(link_tag)) {
        LOG_ERROR("ASSET", "Link-Tag zu lang");
        return false;
    }

    size_t insert_pos = head_close ? (size_t)(head_close - doc) : doc_len;
    size_t link_len = (size_t)ln;

    size_t new_size = doc_len + link_len + 1;
    char *out = (char*)malloc(new_size);
    if (!out) {
        LOG_ERROR("ASSET", "Allocation failed");
        return false;
    }
    cweb_leak_tracker_record("asset.out", out, new_size, true);

    memcpy(out, doc, insert_pos);
    memcpy(out + insert_pos, link_tag, link_len);
    memcpy(out + insert_pos + link_len, doc + insert_pos, doc_len - insert_pos);
    out[new_size - 1] = '\0';

    if (*html) cweb_leak_tracker_record("html.old", *html, strlen(*html) + 1, false);
    free(*html);
    *html = out;

    return true;
}