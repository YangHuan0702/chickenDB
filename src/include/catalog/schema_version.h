//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <cstdint>
#include <vector>

#include "catalog/col_def.h"
#include "catalog/table_catalog.h"

namespace chickenDB {
    struct SchemaVersion {
        uint32_t version{1}; // 从 1 开始递增
        uint64_t effective_ts{0}; // 这个版本从什么时候生效
        uint32_t prev_schema_page_id{CATALOG_NULL_PAGE}; // 上一个版本的 Page（链表）
        uint16_t col_count{0};
        uint8_t reserved[6]{};
    };

    class SchemaPage {
    public:
        auto AddColumn(const ColDef &column) -> void {
            columns_.push_back(column);
            version_.col_count = static_cast<uint16_t>(columns_.size());
        }

        SchemaVersion version_;
        std::vector<ColDef> columns_;
    };
}
