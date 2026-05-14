//
// Created by huan.yang on 2026-05-11.
//

#pragma once
#include <vector>

#include "common/types.h"

namespace chickenDB {

    class LruManager {
    public:
        explicit LruManager(size_t capacity) : capacity_(capacity) {}
        virtual ~LruManager() = default;

        virtual auto Pin(frame_id_t frame_id) -> void = 0;

        virtual auto Unpin(frame_id_t frame_id) -> void = 0;

        virtual auto Evict() -> frame_id_t = 0;

        size_t capacity_;

    };

}
