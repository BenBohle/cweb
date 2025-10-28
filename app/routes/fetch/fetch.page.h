#ifndef FETCH_PAGE_H
#define FETCH_PAGE_H

#include <cweb/cweb.h>
#include "../../../build/fetch.template.h"
#include <cweb/fetch.h>
#include <cweb/cwagger.h>

void fetch_page(Request *req, Response *res);

struct github_response_data
{
    char *id;
    char *name;
    char *html_url;
    // Add more fields as needed
};


#endif // FETCH_PAGE_H
