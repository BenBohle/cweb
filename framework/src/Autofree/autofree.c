#include <cweb/autofree.h>
#include <cweb/logger.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// The pointer passed to cleanup_free is a pointer to the variable itself.
// So we need to dereference it twice to get the actual pointer to free.
void cweb_cleanup_free(void *p) {
	LOG_DEBUG("AUTO", "Cleanup_free called");
    void **ptr = (void **)p;
    if (*ptr) {
        free(*ptr);
		*ptr = NULL;
    }
}

void cweb_cleanup_free_ptrptr(void **p) {
    if (p && *p) {
        free(*p);
        *p = NULL;
        LOG_DEBUG("AUTO", "** freed by cleanup_free_ptrptr");
    }
}

void cweb_cleanup_close_dir(DIR **dp) {
    if (dp && *dp) {
        closedir(*dp);
		*dp = NULL;
    }
}

void cweb_cleanup_close_file(FILE **fp) {
    if (*fp) {
        fclose(*fp);
		*fp = NULL;
    }
}

void cweb_cleanup_close_socket(int *sockfd) {
    if (sockfd && *sockfd >= 0) {
        close(*sockfd);
		*sockfd = -1;
    }
}