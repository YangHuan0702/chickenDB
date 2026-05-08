//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <cstdint>

#include "buffer/page.h"

namespace chickenDB {
    // struct TableCatalogEntry {
    //     uint64_t table_id; // 全局唯一表 ID
    //     char table_name[64]; // 表名，定长简化实现
    //     uint32_t schema_page_id; // 最新 Schema Page
    //     uint32_t segment_index_page_id; // Segment 索引入口
    //     uint64_t row_count; // 当前总行数
    //     uint64_t create_ts;
    //     uint64_t drop_ts; // 0 表示未删除
    //     uint8_t status; // ACTIVE / DROPPED
    //     uint64_t reserved;
    // };

    class TableCatalogEntry : public Page {
    public:
        uint64_t table_id; // 全局唯一表 ID
        char table_name[64]; // 表名，定长简化实现
        uint32_t schema_page_id; // 最新 Schema Page
        uint32_t segment_index_page_id; // Segment 索引入口
        uint64_t row_count; // 当前总行数
        uint64_t create_ts;
        uint64_t drop_ts; // 0 表示未删除
        uint8_t status; // ACTIVE / DROPPED
        uint64_t reserved;
    };
}
