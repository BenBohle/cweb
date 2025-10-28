#include <cweb/compress.h>
#include "compress_internal.h"
#include <brotli/encode.h>

int cweb_brotli(const char* input, size_t input_len, char** output, size_t* output_len) {
    if (!input || !output || !output_len) {
        LOG_ERROR("COMPRESS", "Invalid arguments to compress_with_brotli");
        return -1;
    }

    size_t max_output_len = BrotliEncoderMaxCompressedSize(input_len);
    *output = (char*)malloc(max_output_len);
    if (!*output) {
        LOG_ERROR("COMPRESS", "Memory allocation failed for Brotli output buffer");
        return -1;
    }

    BROTLI_BOOL result = BrotliEncoderCompress(
        BROTLI_DEFAULT_QUALITY, // Compression quality (default level)
        BROTLI_DEFAULT_WINDOW,  // Default window size
        BROTLI_MODE_GENERIC,    // Compression mode
        input_len,              // Input size
        (const uint8_t*)input,  // Input buffer
        &max_output_len,        // In/Out: Compressed size
        (uint8_t*)*output       // Output buffer
    );

    if (result == BROTLI_FALSE) {
        LOG_ERROR("COMPRESS", "Brotli compression failed");
        free(*output);
        *output = NULL;
        return -1;
    }

    *output_len = max_output_len;
    LOG_INFO("COMPRESS", "Brotli compression succeeded: input_len=%zu, output_len=%zu", input_len, *output_len);
    return 0;
}