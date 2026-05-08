//
// Created by huan.yang on 2026-05-08.
//
#include "disk/file_lru_manager.h"

#include "common/chicken_execption.h"

using namespace chickenDB;


FileLruManager::FileLruManager(size_t capacity) : frames_(capacity) {
}

auto FileLruManager::Pin(size_t frame_id) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    ChickenException::AssertCondition(frame_id < frames_.size(), "file frame_id out of range");

    auto &frame = frames_[frame_id];
    if (frame.pin_count == 0 && frame.evictable) {
        evictable_frames_.erase(evictable_iters_[frame_id]);
        evictable_iters_.erase(frame_id);
        frame.evictable = false;
    }
    frame.pin_count++;
}

auto FileLruManager::Unpin(size_t frame_id) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    ChickenException::AssertCondition(frame_id < frames_.size(), "file frame_id out of range");

    auto &frame = frames_[frame_id];
    ChickenException::AssertCondition(frame.pin_count > 0, "file frame is not pinned");

    frame.pin_count--;
    if (frame.pin_count == 0 && !frame.evictable) {
        evictable_frames_.push_front(frame_id);
        evictable_iters_[frame_id] = evictable_frames_.begin();
        frame.evictable = true;
    }
}

auto FileLruManager::GetPinCount(size_t frame_id) -> size_t {
    std::lock_guard<std::mutex> lock(mutex_);
    ChickenException::AssertCondition(frame_id < frames_.size(), "file frame_id out of range");
    return frames_[frame_id].pin_count;
}


auto FileLruManager::Evict() -> size_t {
    std::lock_guard<std::mutex> lock(mutex_);
    ChickenException::AssertCondition(!evictable_frames_.empty(), "don't have available file frame_id");

    const auto frame_id = evictable_frames_.back();
    evictable_frames_.pop_back();
    evictable_iters_.erase(frame_id);
    frames_[frame_id].evictable = false;
    return frame_id;
}
