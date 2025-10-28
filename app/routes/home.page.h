#ifndef _HOME_PAGE_H
#define _HOME_PAGE_H

#include <cweb/cweb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "../../build/home.template.h"

// Function declaration
void home_page(Request *req, Response *res);

#endif // _HOME_PAGE_H
