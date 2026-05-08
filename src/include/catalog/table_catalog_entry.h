//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include "common/types.h"

namespace chickenDB {
    constexpr size_t TABLE_NAME_MAX_LEN = 64;

    enum class TableCatalogEntryStatus : uint8_t {
        ACTIVE = 1,
        DROPPED = 2,
    };

    struct TableCatalogEntry {
        table_id_t table_id{0}; // 全局唯一表 ID
        char table_name[TABLE_NAME_MAX_LEN]{}; // 表名，定长简化实现
        uint32_t schema_page_id{0}; // 最新 Schema Page
        uint32_t segment_index_page_id{0}; // Segment 索引入口
        uint64_t row_count{0}; // 当前总行数
        uint64_t create_ts{0};
        uint64_t drop_ts{0}; // 0 表示未删除
        TableCatalogEntryStatus status{TableCatalogEntryStatus::ACTIVE};
        uint8_t reserved[7]{};

        auto GetTableName() const -> std::string {
            return std::string(table_name, strnlen(table_name, TABLE_NAME_MAX_LEN));
        }

        auto SetTableName(const std::string &name) -> void {
            std::memset(table_name, 0, TABLE_NAME_MAX_LEN);
            std::memcpy(table_name, name.data(), std::min(name.size(), TABLE_NAME_MAX_LEN - 1));
        }

        auto IsActive() const -> bool {
            return status == TableCatalogEntryStatus::ACTIVE;
        }
    };
}
