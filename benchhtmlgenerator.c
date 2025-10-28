// ...existing code...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static unsigned long long parse_size(const char *s) {
    char *end = NULL;
    errno = 0;
    unsigned long long val = strtoull(s, &end, 10);
    if (errno) return 0;
    if (!end || *end == '\0') return val;
    if (end[0] == 'K' || end[0] == 'k') return val * 1024ULL;
    if (end[0] == 'M' || end[0] == 'm') return val * 1024ULL * 1024ULL;
    return 0;
}

/* Kurz: 1KB=1024, 5KB=5120, 10KB=10240, 50KB=51200, 200KB=204800, 1000KB=1024000 */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytes|Nsuffix(K/M)>\n", argv[0]);
        return 1;
    }

    unsigned long long total = parse_size(argv[1]);
    if (total == 0) {
        fprintf(stderr, "Invalid size: %s\n", argv[1]);
        return 1;
    }

    const size_t CHUNK = 8192;
    char buf[CHUNK];
    memset(buf, 'a', sizeof(buf));

    unsigned long long remaining = total;
    while (remaining > 0) {
        size_t to_write = (remaining > CHUNK) ? CHUNK : (size_t)remaining;
        ssize_t written = 0;
        char *p = buf;
        while (to_write > 0) {
            ssize_t n = write(STDOUT_FILENO, p, to_write);
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("write");
                return 1;
            }
            written += n;
            p += n;
            to_write -= n;
            remaining -= n;
        }
    }

    return 0;
}