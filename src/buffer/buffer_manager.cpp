//
// Created by huan.yang on 2026-05-13.
//
#include "buffer/buffer_manager.h"
#include "buffer/clock_sweep_lru.h"
#include "common/chicken_execption.h"
#include <cstring>

using namespace chickenDB;

BufferManager::BufferManager(LRUTableManager *table_manager, size_t capacity)
    : table_manager_(table_manager),
      capacity_(capacity),
      pages_(capacity),
      pin_count_(capacity, 0),
      dirty_(capacity, false) {
    lru_manager_ = std::make_unique<ClockSweepLRU>(capacity);
    for (size_t i = 0; i < capacity; i++) {
        pages_[i] = std::make_unique<Page>();
        pages_[i]->page_id_ = INVALID_PAGE_ID;
        free_list_.push_back(i);
    }
}

BufferManager::~BufferManager() {
    for (auto &[pid, frame_id] : page_table_) {
        if (dirty_[frame_id]) {
            auto fd = table_manager_->Acquire(pid.table_id);
            fd->disk_manager_->WritePage(pid.page_no, pages_[frame_id].get());
            table_manager_->Release(pid.table_id);
        }
    }
}

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
        lru_manager_->Pin(victim);
    }
}

// 调用前须持有 mutex_
auto BufferManager::EvictFrame(frame_id_t frame_id) -> void {
    const PageId &old_pid = pages_[frame_id]->page_id_;
    if (!old_pid.IsValid()) {
        return;
    }
    if (dirty_[frame_id]) {
        auto fd = table_manager_->Acquire(old_pid.table_id);
        fd->disk_manager_->WritePage(old_pid.page_no, pages_[frame_id].get());
        table_manager_->Release(old_pid.table_id);
        dirty_[frame_id] = false;
    }
    page_table_.erase(old_pid);
    pages_[frame_id]->page_id_ = INVALID_PAGE_ID;
}

auto BufferManager::FetchPage(table_id_t table_id, page_id_t page_no) -> Page * {
    std::lock_guard<std::mutex> lock(mutex_);

    PageId pid{table_id, page_no};
    auto it = page_table_.find(pid);
    if (it != page_table_.end()) {
        auto frame_id = it->second;
        pin_count_[frame_id]++;
        lru_manager_->Pin(frame_id);
        return pages_[frame_id].get();
    }

    auto frame_id = FindVictimFrame();
    EvictFrame(frame_id);

    auto fd = table_manager_->Acquire(table_id);
    fd->disk_manager_->ReadPage(page_no, pages_[frame_id].get());
    table_manager_->Release(table_id);

    pages_[frame_id]->page_id_ = pid;
    page_table_[pid] = frame_id;
    pin_count_[frame_id] = 1;
    dirty_[frame_id] = false;
    lru_manager_->Pin(frame_id);
    return pages_[frame_id].get();
}

auto BufferManager::UnpinPage(table_id_t table_id, page_id_t page_no, bool is_dirty) -> bool {
    std::lock_guard<std::mutex> lock(mutex_);

    PageId pid{table_id, page_no};
    auto it = page_table_.find(pid);
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

auto BufferManager::FlushPage(table_id_t table_id, page_id_t page_no) -> bool {
    std::lock_guard<std::mutex> lock(mutex_);

    PageId pid{table_id, page_no};
    auto it = page_table_.find(pid);
    if (it == page_table_.end()) {
        return false;
    }
    auto frame_id = it->second;
    auto fd = table_manager_->Acquire(table_id);
    fd->disk_manager_->WritePage(page_no, pages_[frame_id].get());
    table_manager_->Release(table_id);
    dirty_[frame_id] = false;
    return true;
}

auto BufferManager::NewPage(table_id_t table_id) -> Page * {
    std::lock_guard<std::mutex> lock(mutex_);

    auto frame_id = FindVictimFrame();
    EvictFrame(frame_id);

    // 分配该表的下一个本地页号
    auto &local_next = next_page_no_[table_id];
    page_id_t page_no = local_next++;

    PageId pid{table_id, page_no};
    std::memset(pages_[frame_id]->data, 0, PAGE_SIZE);
    pages_[frame_id]->page_id_ = pid;
    page_table_[pid] = frame_id;
    pin_count_[frame_id] = 1;
    dirty_[frame_id] = true;
    lru_manager_->Pin(frame_id);
    return pages_[frame_id].get();
}

auto BufferManager::InitNextPageNo(table_id_t table_id, page_id_t next_no) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    next_page_no_[table_id] = next_no;
}

auto BufferManager::DeletePage(table_id_t table_id, page_id_t page_no) -> bool {
    std::lock_guard<std::mutex> lock(mutex_);

    PageId pid{table_id, page_no};
    auto it = page_table_.find(pid);
    if (it == page_table_.end()) {
        return true;
    }
    auto frame_id = it->second;
    if (pin_count_[frame_id] > 0) {
        return false;
    }
    page_table_.erase(it);
    dirty_[frame_id] = false;
    pages_[frame_id]->page_id_ = INVALID_PAGE_ID;
    free_list_.push_back(frame_id);
    return true;
}
