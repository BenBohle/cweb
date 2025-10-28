#include <cweb/compress.h>
#include "compress_internal.h"
#include <zlib.h>

int cweb_gzip(const char* input, size_t input_len, char** output, size_t* output_len) {
    if (!input || !output || !output_len) {
        LOG_ERROR("COMPRESS", "Invalid arguments to gzip");
        return -1;
    }

    // Worst-case Abschätzung
    size_t cap = compressBound(input_len) + 32;
    *output = (char*)malloc(cap);
    if (!*output) {
        LOG_ERROR("COMPRESS", "Allocation failed for gzip buffer");
        return -1;
    }

    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    // 15 + 16 -> GZIP Header+Footer
    int ret = deflateInit2(&strm,
                           Z_BEST_COMPRESSION,
                           Z_DEFLATED,
                           15 + 16,
                           8,
                           Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        LOG_ERROR("COMPRESS", "deflateInit2 failed (%d)", ret);
        free(*output);
        *output = NULL;
        return -1;
    }

    strm.next_in = (Bytef*)input;
    strm.avail_in = (uInt)input_len;
    strm.next_out = (Bytef*)*output;
    strm.avail_out = (uInt)cap;

    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        LOG_ERROR("COMPRESS", "deflate failed (%d)", ret);
        deflateEnd(&strm);
        free(*output);
        *output = NULL;
        return -1;
    }

    *output_len = strm.total_out;
    deflateEnd(&strm);

    LOG_INFO("COMPRESS", "Gzip compression succeeded: %zu -> %zu", input_len, *output_len);
    return 0;
}