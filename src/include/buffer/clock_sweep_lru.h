//
// Created by huan.yang on 2026-05-14.
//
#pragma once
#include <atomic>

#include "lru_manager.h"

namespace chickenDB {

    class ClockSweepLRU : public LruManager {
    public:
        explicit ClockSweepLRU(size_t capacity) : LruManager(capacity),frames_(capacity) {
            for (size_t i = 0; i < capacity; i++) {
                frames_[i].store(0);
            }
        }
        ~ClockSweepLRU() override = default;

        auto Pin(frame_id_t frame_id) -> void override;

        auto Unpin(frame_id_t frame_id) -> void override;

        auto Evict() -> frame_id_t override;

    private:
        std::vector<std::atomic<size_t>> frames_;
    };

}
