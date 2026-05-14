//
// Created by huan.yang on 2026-05-11.
//

#pragma once
#include <memory>
#include <vector>

#include "catalog/schema_version.h"
#include "common/macro.h"

namespace chickenDB {
    struct ColumnData {
        char *data;
        uint8_t null_bitmap;
    };


    class Chunk {
    public:
        explicit Chunk() = default;

        ~Chunk() = default;

        Chunk(const Chunk &) = delete;

        Chunk &operator=(const Chunk &) = delete;
        Chunk(Chunk &&) = default;
        Chunk &operator=(Chunk &&) = default;

    };
}
