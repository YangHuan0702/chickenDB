//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "lru_manager.h"
#include "page.h"
#include "disk/disk_manager.h"
#include "common/macro.h"

namespace chickenDB {

    class BufferManager {
    public:
        BufferManager(DiskManager *disk_manager, size_t capacity = K_DEFAULT_CAPACITY);
        ~BufferManager();

        // 从 buffer pool 获取页面，不在 pool 中则从磁盘加载
        auto FetchPage(page_id_t page_id) -> Page *;

        // 释放对页面的引用，is_dirty 表示是否修改过
        auto UnpinPage(page_id_t page_id, bool is_dirty) -> bool;

        // 强制将指定页刷盘
        auto FlushPage(page_id_t page_id) -> bool;

        // 分配一个新页，返回已 pin 的页指针
        auto NewPage() -> Page *;

        // 从 buffer pool 和磁盘删除一个页
        auto DeletePage(page_id_t page_id) -> bool;

        std::unique_ptr<LruManager> lru_manager_;

    private:
        auto FindVictimFrame() -> frame_id_t;

        DiskManager *disk_manager_;
        size_t capacity_;
        std::atomic<page_id_t> next_page_id_{0};

        std::vector<std::unique_ptr<Page>> pages_;
        std::vector<int> pin_count_;
        std::vector<bool> dirty_;
        std::unordered_map<page_id_t, frame_id_t> page_table_;
        std::list<frame_id_t> free_list_;
        std::mutex mutex_;
    };

}
