//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <cstdint>
#include <vector>

#include "buffer/page.h"
#include "table_catalog_entry.h"

namespace chickenDB {
    // 0 在 catalog 链表中用作"无下一页"哨兵（page 0 是 root meta，永远不会是 catalog 链节点）
    constexpr uint32_t CATALOG_NULL_PAGE = 0;

    struct TableCatalogPageHeader {
        uint32_t next_page_id{CATALOG_NULL_PAGE}; // 0 表示最后一页
        uint16_t entry_count{0};
        uint8_t reserved[10]{};
    };

    class TableCatalogPage : public Page {
    public:
        auto GetNextPageId() const -> uint32_t {
            return header_.next_page_id;
        }

        auto SetNextPageId(uint32_t page_id) -> void {
            header_.next_page_id = page_id;
        }

        auto GetEntryCount() const -> uint16_t {
            return header_.entry_count;
        }

        auto AddEntry(const TableCatalogEntry &entry) -> void {
            entries_.push_back(entry);
            header_.entry_count = static_cast<uint16_t>(entries_.size());
        }

        TableCatalogPageHeader header_;
        std::vector<TableCatalogEntry> entries_;
    };
}
