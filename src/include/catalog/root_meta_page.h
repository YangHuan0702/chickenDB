//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <mutex>

#include "buffer/page.h"

namespace chickenDB {
    struct RootMetaPageStruct {
        // 文件标识
        uint64_t  magic;          // "OLAP\x00\x01\x00\x00"
        uint16_t version_major;     // 文件格式主版本
        uint16_t version_minor;

        // 核心目录指针
        uint32_t catalog_page_id;   // Table Catalog 所在 Page
        uint32_t free_list_page_id; // 空闲 Page 链表头

        // 全局统计
        uint64_t total_pages;       // 文件总 Page 数
        uint64_t create_timestamp;
        uint64_t last_checkpoint_ts;

        // 事务/一致性
        uint64_t committed_txn_id;  // 最后提交的事务 ID
    };

    class RootMetaPage : public Page {
    public:
        explicit RootMetaPage() = default;

        auto GetMagic() const -> uint64_t;

        auto GetVersionMajor() const -> uint16_t;
        auto SetVersionMajor(uint16_t) -> void;
        auto GetVersionMinor() const -> uint16_t;
        auto SetVersionMinor(uint16_t) -> void;
        auto GetCatalogPageId() const -> uint64_t;
        auto SetCatalogPageId(uint64_t) -> void;
        auto GetFreeListPageId() const -> uint64_t;
        auto SetFreeListPageId(uint64_t) -> void;
        auto GetTotalPages() const -> uint64_t;
        auto SetTotalPages(uint64_t) -> void;
        auto GetCreateTimestamp() const -> uint64_t;
        auto SetCreateTimestamp(uint64_t) -> void;
        auto GetLastCheckpointTs() const -> uint64_t;
        auto SetLastCheckpointTs(uint64_t) -> void;
        auto GetCommittedTxnId() const -> uint64_t;
        auto SetCommittedTxnId(uint64_t) -> void;

        std::mutex mutex_;
        RootMetaPageStruct info_;
    };
}
