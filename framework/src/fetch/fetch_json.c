#include <cweb/fetch.h>
#include "cweb_fetch_internal.h"
#include <stdlib.h>
#include <string.h>

/* JSON API implementation using cJSON */

fetch_json_t *fetch_json_create_object(void) {
    fetch_json_t *json = malloc(sizeof(fetch_json_t));
    if (!json) {
        return NULL;
    }
    
    json->cjson = cJSON_CreateObject();
    if (!json->cjson) {
        free(json);
        return NULL;
    }
    
    return json;
}

fetch_json_t *fetch_json_create_array(void) {
    fetch_json_t *json = malloc(sizeof(fetch_json_t));
    if (!json) {
        return NULL;
    }
    
    json->cjson = cJSON_CreateArray();
    if (!json->cjson) {
        free(json);
        return NULL;
    }
    
    return json;
}

fetch_json_t *fetch_json_parse(const char *json_string) {
    if (!json_string) {
        return NULL;
    }
    
    fetch_json_t *json = malloc(sizeof(fetch_json_t));
    if (!json) {
        return NULL;
    }
    
    json->cjson = cJSON_Parse(json_string);
    if (!json->cjson) {
        free(json);
        return NULL;
    }
    
    return json;
}

char *fetch_json_to_string(const fetch_json_t *json) {
    if (!json || !json->cjson) {
        return NULL;
    }
    
    return cJSON_Print(json->cjson);
}

char *fetch_json_to_string_pretty(const fetch_json_t *json) {
    if (!json || !json->cjson) {
        return NULL;
    }
    
    return cJSON_Print(json->cjson);
}

void fetch_json_destroy(fetch_json_t *json) {
    if (!json) return;
    
    if (json->cjson) {
        cJSON_Delete(json->cjson);
    }
    
    free(json);
}

/* Object operations */
fetch_error_t fetch_json_object_set_string(fetch_json_t *json,
                                               const char *key,
                                               const char *value) {
    if (!json || !json->cjson || !key || !value) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    cJSON *item = cJSON_CreateString(value);
    if (!item) {
        return FETCH_ERROR_MEMORY;
    }
    
    if (!cJSON_AddItemToObject(json->cjson, key, item)) {
        cJSON_Delete(item);
        return FETCH_ERROR_JSON;
    }
    
    return FETCH_OK;
}

fetch_error_t fetch_json_object_set_number(fetch_json_t *json,
                                               const char *key,
                                               double value) {
    if (!json || !json->cjson || !key) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    cJSON *item = cJSON_CreateNumber(value);
    if (!item) {
        return FETCH_ERROR_MEMORY;
    }
    
    if (!cJSON_AddItemToObject(json->cjson, key, item)) {
        cJSON_Delete(item);
        return FETCH_ERROR_JSON;
    }
    
    return FETCH_OK;
}

fetch_error_t fetch_json_object_set_bool(fetch_json_t *json,
                                             const char *key,
                                             bool value) {
    if (!json || !json->cjson || !key) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    cJSON *item = cJSON_CreateBool(value);
    if (!item) {
        return FETCH_ERROR_MEMORY;
    }
    
    if (!cJSON_AddItemToObject(json->cjson, key, item)) {
        cJSON_Delete(item);
        return FETCH_ERROR_JSON;
    }
    
    return FETCH_OK;
}

fetch_error_t fetch_json_object_set_null(fetch_json_t *json,
                                             const char *key) {
    if (!json || !json->cjson || !key) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    cJSON *item = cJSON_CreateNull();
    if (!item) {
        return FETCH_ERROR_MEMORY;
    }
    
    if (!cJSON_AddItemToObject(json->cjson, key, item)) {
        cJSON_Delete(item);
        return FETCH_ERROR_JSON;
    }
    
    return FETCH_OK;
}

const char *fetch_json_object_get_string(const fetch_json_t *json,
                                           const char *key) {
    if (!json || !json->cjson || !key) {
        return NULL;
    }
    
    cJSON *item = cJSON_GetObjectItem(json->cjson, key);
    if (!item || !cJSON_IsString(item)) {
        return NULL;
    }
    
    return cJSON_GetStringValue(item);
}

double fetch_json_object_get_number(const fetch_json_t *json,
                                      const char *key) {
    if (!json || !json->cjson || !key) {
        return 0.0;
    }
    
    cJSON *item = cJSON_GetObjectItem(json->cjson, key);
    if (!item || !cJSON_IsNumber(item)) {
        return 0.0;
    }
    
    return cJSON_GetNumberValue(item);
}

bool fetch_json_object_get_bool(const fetch_json_t *json,
                                  const char *key) {
    if (!json || !json->cjson || !key) {
        return false;
    }
    
    cJSON *item = cJSON_GetObjectItem(json->cjson, key);
    if (!item || !cJSON_IsBool(item)) {
        return false;
    }
    
    return cJSON_IsTrue(item);
}

/* Array operations */
fetch_error_t fetch_json_array_add_string(fetch_json_t *json,
                                              const char *value) {
    if (!json || !json->cjson || !value) {
        return FETCH_ERROR_INVALID_PARAM;
    }
    
    cJSON *item = cJSON_CreateString(value);
    if (!item) {
        return FETCH_ERROR_MEMORY;
    }
    
    if (!cJSON_AddItemToArray(json->cjson, item)) {
        cJSON_Delete(item);
        return FETCH_ERROR_JSON;
    }
    
    return FETCH_OK;
}

size_t fetch_json_array_size(const fetch_json_t *json) {
    if (!json || !json->cjson || !cJSON_IsArray(json->cjson)) {
        return 0;
    }
    
    return cJSON_GetArraySize(json->cjson);
}
