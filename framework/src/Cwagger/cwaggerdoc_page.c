// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025 Ben Bohle
 * Licensed under the Apache License, Version 2.0
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "cwaggerdoc_page.h"
#include <cweb/cweb.h>
#include <cweb/cwagger.h>
#include "cwagger.template.h"

void cweb_cwaggerdoc_page(Request *req, Response *res) {

	if (strcmp(req->method, "GET") != 0) { 
		res->status_code = 403;
		res->body = "Method not allowed";
		res->body_len = strlen(res->body);
		cweb_add_response_header(res, "Content-Type", "text/plain");
		return;
	}


	cwagger_endpoint *apidoc = cwagger_get_endpoints();
	size_t count = cwagger_get_endpoint_count();

	LOG_DEBUG("CWAGGERDOC", "Generating API docs for %zu endpoints", cwagger_get_endpoint_count());
	char *html = cwagger_template(apidoc, count);
	LOG_DEBUG("CWAGGERDOC", "Generated HTML length: %zu", html ? strlen(html) : 0);
	res->body = strdup(html);
	free(html);
	res->body_len = strlen(res->body);
	cweb_add_response_header(res, "Content-Type", "text/html");
	res->status_code = 200;
	res->state = PROCESSED;
	return;
}

