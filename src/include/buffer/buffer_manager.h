//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
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
        explicit BufferManager(std::shared_ptr<LRUTableManager> table_manager, size_t capacity = K_DEFAULT_CAPACITY);
        ~BufferManager();

        // 从 buffer pool 获取页面，不在 pool 中则从磁盘加载
        auto FetchPage(table_id_t table_id, page_id_t page_no) -> Page *;

        // 重启后从持久化元数据恢复某张表的页号计数器
        auto InitNextPageNo(table_id_t table_id, page_id_t next_no) -> void;

        // 释放对页面的引用，is_dirty 表示是否修改过
        auto UnpinPage(table_id_t table_id, page_id_t page_no, bool is_dirty) -> bool;

        // 强制将指定页刷盘
        auto FlushPage(table_id_t table_id, page_id_t page_no) -> bool;

        // 将 buffer pool 中所有脏页刷盘（checkpoint 入口）
        auto FlushAllDirtyPages() -> void;

        // 为指定表分配新页，返回已 pin 的页指针
        auto NewPage(table_id_t table_id) -> Page *;

        // 返回该表已分配的页数（本地页号计数器）。扫描时用于推导页范围。
        auto GetPageCount(table_id_t table_id) -> page_id_t;

        // 从 buffer pool 和磁盘删除一个页
        auto DeletePage(table_id_t table_id, page_id_t page_no) -> bool;

        std::unique_ptr<LruManager> lru_manager_;

    private:
        auto FindVictimFrame() -> frame_id_t;

        // 将 frame 刷盘并从 page_table_ 中移除，调用前须持有 mutex_
        auto EvictFrame(frame_id_t frame_id) -> void;

        // 刷写所有 pin_count==0 的脏帧，每帧持锁写盘后释放锁
        auto FlushDirtyUnpinned() -> void;

        // 后台刷盘线程主循环
        auto BackgroundFlush() -> void;

        std::shared_ptr<LRUTableManager> table_manager_;
        size_t capacity_ [[maybe_unused]];

        // per-table 本地页号计数器（受 mutex_ 保护）
        std::unordered_map<table_id_t, page_id_t> next_page_no_;

        std::vector<std::unique_ptr<Page>> pages_;
        std::vector<int> pin_count_;
        std::vector<bool> dirty_;
        std::unordered_map<PageId, frame_id_t, PageIdHash> page_table_;
        std::list<frame_id_t> free_list_;
        std::mutex mutex_;

        // 后台刷盘控制
        std::atomic<size_t> dirty_count_{0};
        std::atomic<bool> shutdown_{false};
        std::mutex flush_mutex_;
        std::condition_variable flush_cv_;
        std::thread flush_thread_;
    };

}
