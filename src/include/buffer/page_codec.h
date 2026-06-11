//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include "zstd.h"
#include "common/enum/compression_type.h"
#include "common/chicken_execption.h"

namespace chickenDB {
    // Thin codec layer isolating the zstd dependency from the page format.
    // The storage layer only ever talks to PageCodec, so swapping in LZ4/Snappy
    // later is a change confined to this file. Both directions are stateless.
    class PageCodec {
    public:
        // Compress `src_len` bytes from `src` into `out` (resized to fit).
        // Returns the number of bytes written to `out`.
        static auto Compress(CompressionType codec, const char *src, size_t src_len,
                             std::vector<char> &out) -> size_t {
            switch (codec) {
                case CompressionType::NONE: {
                    out.resize(src_len);
                    if (src_len > 0) {
                        std::memcpy(out.data(), src, src_len);
                    }
                    return src_len;
                }
                case CompressionType::ZSTD: {
                    size_t bound = ZSTD_compressBound(src_len);
                    out.resize(bound);
                    size_t written = ZSTD_compress(out.data(), bound, src, src_len, /*level=*/3);
                    ChickenException::AssertCondition(
                        ZSTD_isError(written) == 0,
                        std::string("[PageCodec] zstd compress failed: ") + ZSTD_getErrorName(written));
                    out.resize(written);
                    return written;
                }
            }
            ChickenException::AssertCondition(false, "[PageCodec] unknown compression codec");
            return 0;
        }

        // Decompress `src_len` compressed bytes into `out`, which is sized to the
        // known `raw_size` (carried in the column directory). Returns raw_size.
        static auto Decompress(CompressionType codec, const char *src, size_t src_len,
                               size_t raw_size, std::vector<char> &out) -> size_t {
            switch (codec) {
                case CompressionType::NONE: {
                    out.resize(src_len);
                    if (src_len > 0) {
                        std::memcpy(out.data(), src, src_len);
                    }
                    return src_len;
                }
                case CompressionType::ZSTD: {
                    out.resize(raw_size);
                    size_t got = ZSTD_decompress(out.data(), raw_size, src, src_len);
                    ChickenException::AssertCondition(
                        ZSTD_isError(got) == 0,
                        std::string("[PageCodec] zstd decompress failed: ") + ZSTD_getErrorName(got));
                    ChickenException::AssertCondition(
                        got == raw_size, "[PageCodec] decompressed size mismatch");
                    return got;
                }
            }
            ChickenException::AssertCondition(false, "[PageCodec] unknown compression codec");
            return 0;
        }
    };
}
