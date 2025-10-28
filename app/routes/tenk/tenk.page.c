#include "tenk.page.h"

void tenk_page(Request *req, Response *res) {

        if (strcmp(req->method, "POST") == 0) { 
                res->status_code = 403;
                res->body = "Method not allowed";
                res->body_len = strlen(res->body);
                cweb_add_response_header(res, "Content-Type", "text/plain");
                return;
        }

        res->status_code = 200;
        res->body = template_tenk();
        res->body_len = strlen(res->body);
        cweb_add_response_header(res, "Content-Type", "text/plain");
        res->state = PROCESSED;
        return;
}

