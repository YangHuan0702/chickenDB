//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <cstdint>
#include <vector>

#include "buffer/page.h"
#include "table_catalog_entry.h"

namespace chickenDB {
    constexpr uint32_t INVALID_PAGE_ID = 0;

    struct TableCatalogPageHeader {
        uint32_t next_page_id{INVALID_PAGE_ID}; // 0 表示最后一页
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
