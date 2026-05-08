//
// Created by huan.yang on 2026-05-08.
//
#include "disk/file_lru_manager.h"

#include "common/chicken_execption.h"

using namespace chickenDB;


FileLruManager::FileLruManager(size_t capacity) {
    for (size_t i = 0; i < capacity; i++) {
        queue_.push_back(i);
    }
}

auto FileLruManager::Pin(size_t frame_id) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    tables_[frame_id]++;
}

auto FileLruManager::Unpin(size_t frame_id) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    tables_[frame_id]--;
}

auto FileLruManager::GetPinCount(size_t frame_id) -> size_t {
    std::lock_guard<std::mutex> lock(mutex_);
    return tables_[frame_id];
}


auto FileLruManager::Evict() -> size_t {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = queue_.rbegin(); it != queue_.rend(); it++) {
        if (tables_[*it] <= 0) {
            return *it;
        }
    }
    throw ChickenException("don`t have avali file frame_id");
}
