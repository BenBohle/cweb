#include "fetch.page.h"
#include <cweb/fetch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <cweb/server.h>

// Global variables for async handling
static fetch_data_t *g_data = NULL;
static Response *g_response = NULL;

void fetch_page_assets() {
   
	cwagger_detail detail = {
		.request_schema = "{}",
		.response_schema = "{ \"type\": \"object\", \"properties\": { \"login\": { \"type\": \"string\" }, \"id\": { \"type\": \"integer\" }, \"avatar_url\": { \"type\": \"string\" }, \"html_url\": { \"type\": \"string\" }, \"type\": { \"type\": \"string\" }, \"public_repos\": { \"type\": \"integer\" }, \"followers\": { \"type\": \"integer\" }, \"following\": { \"type\": \"integer\" } }, \"required\": [\"login\", \"id\", \"avatar_url\", \"html_url\", \"type\", \"public_repos\", \"followers\", \"following\"] }",
		.expected_arguments = "{}"
	};
	cwagger_add("GET", "/fetch", "custom fetch test", &detail);

}
REGISTER_FRONTEND_ASSET(fetch_page_assets);

void github_response_callback(fetch_request_t *request, fetch_response_t *response, void *user_data) {
    LOG_DEBUG("FETCH_PAGE", "GitHub API response received in callback");
    fetch_data_t *data = (fetch_data_t *)user_data;
    
    if (fetch_response_get_status(response) == 200) {
        const fetch_json_t *json = fetch_response_get_json(response);
        if (json) {
            // Extract GitHub profile data from JSON
            const char *login = fetch_json_object_get_string(json, "login");
            const char *avatar_url = fetch_json_object_get_string(json, "avatar_url");
            const char *html_url = fetch_json_object_get_string(json, "html_url");
            const char *type = fetch_json_object_get_string(json, "type");
            
            // Update data structure
            if (login) data->login = strdup(login);
            if (avatar_url) data->avatar_url = strdup(avatar_url);
            if (html_url) data->html_url = strdup(html_url);
            if (type) data->type = strdup(type);
            
            data->id = (int)fetch_json_object_get_number(json, "id");
            data->public_repos = (int)fetch_json_object_get_number(json, "public_repos");
            data->followers = (int)fetch_json_object_get_number(json, "followers");
            data->following = (int)fetch_json_object_get_number(json, "following");
            
            LOG_DEBUG("FETCH_PAGE", "GitHub API data successfully retrieved");
        }
    } else {
        LOG_DEBUG("FETCH_PAGE", "GitHub API request failed");
        // Set default values on failure
        data->login = strdup("octocat");
        data->id = 583231;
        data->avatar_url = strdup("https://github.com/images/error/octocat_happy.gif");
        data->html_url = strdup("https://github.com/octocat");
        data->type = strdup("User");
        data->public_repos = 8;
        data->followers = 4000;
        data->following = 9;
    }
    
    // Print extracted data
    printf("Login: %s\n", data->login);
    printf("ID: %d\n", data->id);
    printf("Avatar URL: %s\n", data->avatar_url);
    printf("HTML URL: %s\n", data->html_url);
    printf("Type: %s\n", data->type);
    printf("Public Repos: %d\n", data->public_repos);
    printf("Followers: %d\n", data->followers);
    printf("Following: %d\n", data->following);

    // Generate HTML template
    char *html = fetch_template(data);

    // Set response
    g_response->status_code = 200;
    g_response->body = html;
    g_response->body_len = strlen(html);
    cweb_add_response_header(g_response, "Content-Type", "text/html");
    
    // Clean up request
    fetch_request_destroy(request);
    
    // Free allocated strings
    free(data->login);
    free(data->avatar_url);
    free(data->html_url);
    free(data->type);
    
    g_response->state = PROCESSED;

    LOG_DEBUG("FETCH_PAGE", "Response successfully fetched and processed");
}

void fetch_page(Request *req, Response *res) {
    if (strcmp(req->method, "POST") == 0) { 
        LOG_DEBUG("FETCH_PAGE", "POST method not allowed for fetch page");
        res->status_code = 403;
        res->body = "Method not allowed";
        res->isliteral = 1;
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
    }
    
    if (fetch_global_init() != FETCH_OK) {
        LOG_DEBUG("FETCH_PAGE", "Failed to initialize fetch library");
        res->status_code = 500;
        res->body = "Internal server error";
        res->isliteral = 1;
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
    }
    
    // Create HTTP client
    fetch_config_t config = fetch_config_default();
    config.user_agent = "CWeb-Framework/1.0";
    fetch_client_t *client = fetch_client_create(g_event_base, &config);
    if (!client) {
        LOG_DEBUG("FETCH_PAGE", "Failed to create HTTP client");
        res->status_code = 500;
        res->body = "Internal server error";
        res->isliteral = 1;
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        fetch_global_cleanup();
        return;
    }
    
    fetch_data_t *data = malloc(sizeof(fetch_data_t));
    memset(data, 0, sizeof(fetch_data_t));
    
    // Set global variables for callback
    g_data = data;
    g_response = res;
    
    // Create GitHub API request (using octocat as example)
    fetch_request_t *request = fetch_request_create(client, FETCH_GET, "https://api.github.com/users/octocat");
    if (!request) {
        LOG_DEBUG("FETCH_PAGE", "Failed to create GitHub API request");
        res->status_code = 500;
        res->body = "Internal server error";
        res->isliteral = 1;
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        free(data);
        fetch_client_destroy(client);
        fetch_global_cleanup();
        return;
    }
    
    // Set GitHub API headers
    fetch_request_set_header(request, "Accept", "application/vnd.github.v3+json");
    fetch_request_set_header(request, "User-Agent", "CWeb-Framework/1.0");
    
    // Set callback
    fetch_request_set_callback(request, github_response_callback, data);
    
    // Execute request
    if (fetch_request_execute(request) != FETCH_OK) {
        LOG_DEBUG("FETCH_PAGE", "Failed to execute GitHub API request");
        res->status_code = 500;
        res->body = "Internal server error";
        res->isliteral = 1;
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        fetch_request_destroy(request);
        free(data);
        fetch_client_destroy(client);
        fetch_global_cleanup();
        return;
    }
    
    LOG_DEBUG("FETCH_PAGE", "GitHub API request initiated, returning to main event loop");
    
    // Response state will be set to PROCESSED by callback when request completes
}
