#include "helloworld.page.h"

void helloworld_page(Request *req, Response *res) {

 char *content = "Hello World!";
         res->status_code = 200;
         res->body = strdup(content);
         res->body_len = strlen(res->body);
         // res->isliteral = 1; // Mark as static content
         cweb_add_response_header(res, "Content-Type", "text/plain");
         res->state = PROCESSED;
         return;
}

