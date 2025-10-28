#include <cweb/cweb.h>
#include <cweb/dev.h>

// void define_routes() {
//     cweb_add_route("/", handle_home, false);
//     cweb_add_route("/about", handle_about, false);
//     cweb_add_route("/load", handle_load_html, false);
//     cweb_add_route("/tufte.css", handle_load_css, false);
// }

int main(int argc, char *argv[]) {
    const char *port = "8080";
    if (argc > 1) {
        port = argv[1];
    }

    cweb_set_mode(CWEB_MODE_PROD);

    // Init Cbase FileCache
    if (cweb_default_fileserver_config(FILESERVER_MODE_HYBRID, 1000 * 1024 * 1024, true) != 0) {
        return 1;
    }



	cwagger_init("/cwagger");

	// root site / currently not implemented cause of an anoying error. i have to fix it first.
	// test tite url: /home --> name == route
	auto_routes(1);
    // cweb_set_fallback_handler(home_page);
	// cweb_add_route("/", home_page, false);
	// cweb_add_route("/fetch", fetch_page, false);

    cweb_call_all_frontend_asset_functions();

    // Initialize WebServer
   	cweb_run_server(port);

    // Cleanup
    cweb_cleanup_server();

    return 0;
}
