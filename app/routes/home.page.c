#include "home.page.h"
#include <cweb/template.h>
#include <cweb/cwagger.h>
#include <cweb/leak_detector.h>

void home_page_css(Request *req, Response *res)
{
    page_data_t data = {0};
    data.page_title = "CWeb Template Engine Demo";
    data.username = "John Developer";
    data.generated_time = time(NULL);
    data.is_admin = true;
    data.user_score = 1337;
    data.status_message = "<b>Welcome to CWeb!</b>";
    
    // Sample menu items
    const char *menu_items[] = {
        "Home", "About", "Services", "Portfolio", "Contact", "Blog"
    };
    data.menu_items = menu_items;
    data.menu_count = sizeof(menu_items) / sizeof(menu_items[0]);

	char *css = NULL;
  	css = home_template_css(&data);

    res->status_code = 200;
	res->body = css;
	res->body_len = strlen(css);
    cweb_leak_tracker_record("home_page_css", res->body, res->body_len, true);

    cweb_add_response_header(res, "Content-Type", "text/css");
	cweb_add_response_header(res, "Cache-Control", "no-chache");
    res->state = PROCESSED;

}


void home_page_assets() {
    cweb_add_route("/home/styles.css", home_page_css, false);
    cweb_add_route("/home/test_scripts.js", home_page, false);
	// cweb_set_dynamic_subpath("/home", 1);
	cweb_set_dynamic_param("/home", 1);
	cwagger_detail detail = {
		.request_schema = "{}",
		.response_schema = "{ \"type\": \"object\", \"properties\": { \"message\": { \"type\": \"string\" } }, \"required\": [\"message\"] }",
		.expected_arguments = "{}"
	};
	cwagger_add("GET", "/home", "Renders the home page with dynamic content using CWeb template engine.", &detail);

}
REGISTER_FRONTEND_ASSET(home_page_assets);

void home_page(Request *req, Response *res) {

	LOG_DEBUG("SESSION", "Session ID: %s", req->session ? req->session->id : "NULL");

    // Create sample data
    page_data_t data = {0};
    data.page_title = "CWeb Template Engine Demo";
    data.username = "John Developer";
    data.generated_time = time(NULL);
    data.is_admin = true;
    data.user_score = 1337;
    data.status_message = "<b>Welcome to CWeb!</b>";
    
    // Sample menu items
    const char *menu_items[] = {
        "Home", "About", "Services", "Portfolio", "Contact", "Blog"
    };
    data.menu_items = menu_items;
    data.menu_count = sizeof(menu_items) / sizeof(menu_items[0]);
    
    // Render the template
    char *html = home_template(&data);
    cweb_setAssetLink(&html, "styles.css", "/home/styles.css", "stylesheet");

	res->status_code = 200;
	res->body = html;
	res->body_len = strlen(html);
    cweb_leak_tracker_record("home_page_html", html, strlen(html), true);

	cweb_add_response_header(res, "Content-Type", "text/html");
	cweb_add_performance_headers(res, "text/html");
    res->state = PROCESSED;

}