//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "lru_manager.h"
#include "page.h"
#include "disk/table_manager.h"
#include "common/macro.h"
#include "common/types.h"

namespace chickenDB {

    class BufferManager {
    public:
        explicit BufferManager(LRUTableManager *table_manager, size_t capacity = K_DEFAULT_CAPACITY);
        ~BufferManager();

        // 从 buffer pool 获取页面，不在 pool 中则从磁盘加载
        auto FetchPage(table_id_t table_id, page_id_t page_no) -> Page *;

        // 释放对页面的引用，is_dirty 表示是否修改过
        auto UnpinPage(table_id_t table_id, page_id_t page_no, bool is_dirty) -> bool;

        // 强制将指定页刷盘
        auto FlushPage(table_id_t table_id, page_id_t page_no) -> bool;

        // 为指定表分配新页，返回已 pin 的页指针
        auto NewPage(table_id_t table_id) -> Page *;

        // 从 buffer pool 和磁盘删除一个页
        auto DeletePage(table_id_t table_id, page_id_t page_no) -> bool;

        std::unique_ptr<LruManager> lru_manager_;

    private:
        auto FindVictimFrame() -> frame_id_t;

        // 将 frame 刷盘并从 page_table_ 中移除，调用前须持有 mutex_
        auto EvictFrame(frame_id_t frame_id) -> void;

        LRUTableManager *table_manager_;
        size_t capacity_;

        // per-table 本地页号计数器（受 mutex_ 保护）
        std::unordered_map<table_id_t, page_id_t> next_page_no_;

        std::vector<std::unique_ptr<Page>> pages_;
        std::vector<int> pin_count_;
        std::vector<bool> dirty_;
        std::unordered_map<PageId, frame_id_t, PageIdHash> page_table_;
        std::list<frame_id_t> free_list_;
        std::mutex mutex_;
    };

}
