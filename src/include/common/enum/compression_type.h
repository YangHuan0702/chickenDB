//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>

namespace chickenDB {
    // Per-page column-block compression codec. Stored in PageHeader.compression
    // (1 byte) so a page is self-describing: the scan path reads this to decide
    // how to decode each column block. NONE keeps the raw bytes verbatim.
    enum class CompressionType : uint8_t {
        NONE = 0,
        ZSTD = 1,
    };
}
