//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "common/types.h"

namespace chickenDB {
    class FileLruManager {
    public:
        explicit FileLruManager(size_t capacity);

        ~FileLruManager() = default;

        auto Pin(size_t frame_id) -> void;

        auto Unpin(size_t frame_id) -> void;

        auto GetPinCount(size_t frame_id) -> size_t;

        auto Evict() -> size_t;

    private:
        struct FrameState {
            size_t pin_count{0};
            bool evictable{false};
        };

        std::vector<FrameState> frames_;
        std::list<size_t> evictable_frames_;
        std::unordered_map<size_t, std::list<size_t>::iterator> evictable_iters_;

        std::mutex mutex_;
    };
}
