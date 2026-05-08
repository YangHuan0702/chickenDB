//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <cstdint>

namespace chickenDB {

    struct SchemaVersion {
        uint32_t version; // 从 1 开始递增
        uint64_t effective_ts; // 这个版本从什么时候生效
        uint32_t prev_schema_page_id; // 上一个版本的 Page（链表）
        uint16_t col_count;
        uint8_t reserved[6];
        // ColDef columns[]; // 紧跟在后面
    };



}
