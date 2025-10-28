#include <cweb/fileserver.h>
#include "fileserver_internal.h"
#include <cweb/leak_detector.h>

// Hilfsfunktionen: robustes Lesen/Schreiben + feste Endianness (LE)
int write_bytes(FILE *f, const void *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        size_t n = fwrite((const char*)buf + off, 1, len - off, f);
        if (n == 0) {
            if (ferror(f)) return -1;
            break;
        }
        off += n;
    }
    return off == len ? 0 : -1;
}
int read_bytes(FILE *f, void *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        size_t n = fread((char*)buf + off, 1, len - off, f);
        if (n == 0) {
            if (feof(f)) break;
            if (ferror(f)) return -1;
        }
        off += n;
    }
    return off == len ? 0 : -1;
}
int write_u32_le(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v), (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24) };
    return write_bytes(f, b, 4);
}
int write_u64_le(FILE *f, uint64_t v) {
    uint8_t b[8] = {
        (uint8_t)(v), (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24),
        (uint8_t)(v>>32), (uint8_t)(v>>40), (uint8_t)(v>>48), (uint8_t)(v>>56)
    };
    return write_bytes(f, b, 8);
}
int read_u32_le(FILE *f, uint32_t *out) {
    uint8_t b[4];
    if (read_bytes(f, b, 4) != 0) return -1;
    *out = (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
    return 0;
}
int read_u64_le(FILE *f, uint64_t *out) {
    uint8_t b[8];
    if (read_bytes(f, b, 8) != 0) return -1;
    *out = (uint64_t)b[0] | ((uint64_t)b[1]<<8) | ((uint64_t)b[2]<<16) | ((uint64_t)b[3]<<24) |
           ((uint64_t)b[4]<<32) | ((uint64_t)b[5]<<40) | ((uint64_t)b[6]<<48) | ((uint64_t)b[7]<<56);
    return 0;
}