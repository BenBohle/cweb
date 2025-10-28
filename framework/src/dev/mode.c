#include <cweb/dev.h>

static CwebMode current_mode = CWEB_MODE_PROD;

void cweb_set_mode(CwebMode mode) {
    current_mode = mode;
}

CwebMode cweb_get_mode(void) {
    return current_mode;
}