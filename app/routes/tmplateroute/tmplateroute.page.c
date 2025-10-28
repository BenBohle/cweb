#include "tmplateroute.page.h"

void tmplateroute_page(Request *req, Response *res) {

	if (strcmp(req->method, "POST") == 0) { 
		res->status_code = 403;
		res->body = "Method not allowed";
		res->body_len = strlen(res->body);
		cweb_add_response_header(res, "Content-Type", "text/plain");
		return;
	}

	// if (strcmp(req->method, "GET") && isLinkedAssetRequest(req->path)) {
	// 	// serve_linked_asset(req, res);
	// 	res->state = PROCESSED;
	// }
}

