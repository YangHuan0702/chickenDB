//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <list>
#include <mutex>
#include <unordered_map>

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
        std::unordered_map<table_id_t, size_t> tables_{};
        std::list<table_id_t> queue_;

        std::mutex mutex_;
    };
}
