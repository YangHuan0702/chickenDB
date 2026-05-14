//
// Created by huan.yang on 2026-05-13.
//
#include "buffer/buffer_manager.h"
#include "buffer/clock_sweep_lru.h"
#include "common/chicken_execption.h"
#include <cstring>

using namespace chickenDB;

BufferManager::BufferManager(DiskManager *disk_manager, size_t capacity)
    : disk_manager_(disk_manager),
      capacity_(capacity),
      pages_(capacity),
      pin_count_(capacity, 0),
      dirty_(capacity, false) {
    lru_manager_ = std::make_unique<ClockSweepLRU>(capacity);
    for (size_t i = 0; i < capacity; i++) {
        pages_[i] = std::make_unique<Page>();
        pages_[i]->page_id_ = -1;
        free_list_.push_back(i);
    }
}

BufferManager::~BufferManager() {
    for (auto &[page_id, frame_id] : page_table_) {
        if (dirty_[frame_id]) {
            disk_manager_->WritePage(page_id, pages_[frame_id].get());
        }
    }
}

// 从 free_list 取或通过 clock sweep 淘汰一个 frame
auto BufferManager::FindVictimFrame() -> frame_id_t {
    if (!free_list_.empty()) {
        auto frame_id = free_list_.front();
        free_list_.pop_front();
        return frame_id;
    }
    while (true) {
        auto victim = lru_manager_->Evict();
        if (pin_count_[victim] == 0) {
            return victim;
        }
        // 该 frame 还有 pinner，拉高时钟计数让它晚点被淘汰
        lru_manager_->Pin(victim);
    }
}

auto BufferManager::FetchPage(page_id_t page_id) -> Page * {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        auto frame_id = it->second;
        pin_count_[frame_id]++;
        lru_manager_->Pin(frame_id);
        return pages_[frame_id].get();
    }

    auto frame_id = FindVictimFrame();
    auto old_page_id = pages_[frame_id]->page_id_;
    if (old_page_id >= 0) {
        if (dirty_[frame_id]) {
            disk_manager_->WritePage(old_page_id, pages_[frame_id].get());
        }
        page_table_.erase(old_page_id);
    }

    disk_manager_->ReadPage(page_id, pages_[frame_id].get());
    pages_[frame_id]->page_id_ = page_id;
    page_table_[page_id] = frame_id;
    pin_count_[frame_id] = 1;
    dirty_[frame_id] = false;
    lru_manager_->Pin(frame_id);
    return pages_[frame_id].get();
}

auto BufferManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }
    auto frame_id = it->second;
    if (pin_count_[frame_id] == 0) {
        return false;
    }
    pin_count_[frame_id]--;
    if (is_dirty) {
        dirty_[frame_id] = true;
    }
    return true;
}

auto BufferManager::FlushPage(page_id_t page_id) -> bool {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }
    auto frame_id = it->second;
    disk_manager_->WritePage(page_id, pages_[frame_id].get());
    dirty_[frame_id] = false;
    return true;
}

auto BufferManager::NewPage() -> Page * {
    std::lock_guard<std::mutex> lock(mutex_);

    auto frame_id = FindVictimFrame();
    auto old_page_id = pages_[frame_id]->page_id_;
    if (old_page_id >= 0) {
        if (dirty_[frame_id]) {
            disk_manager_->WritePage(old_page_id, pages_[frame_id].get());
        }
        page_table_.erase(old_page_id);
    }

    auto page_id = next_page_id_.fetch_add(1);
    std::memset(pages_[frame_id]->data, 0, PAGE_SIZE);
    pages_[frame_id]->page_id_ = page_id;
    page_table_[page_id] = frame_id;
    pin_count_[frame_id] = 1;
    dirty_[frame_id] = true;
    lru_manager_->Pin(frame_id);
    return pages_[frame_id].get();
}

auto BufferManager::DeletePage(page_id_t page_id) -> bool {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return true;
    }
    auto frame_id = it->second;
    if (pin_count_[frame_id] > 0) {
        return false;
    }
    page_table_.erase(it);
    dirty_[frame_id] = false;
    pages_[frame_id]->page_id_ = -1;
    free_list_.push_back(frame_id);
    return true;
}
