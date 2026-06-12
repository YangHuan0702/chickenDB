//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "common/types.h"
#include "index/index.h"

namespace chickenDB {
    // 索引元数据 + 活的索引实例。索引数据本身常驻内存（内存版 B+树/哈希/位图）；
    // 索引「定义」持久化到 catalog，重启后扫表重建（见 IndexDefRecord）。
    struct IndexInfo {
        uint32_t index_id{0};
        std::string index_name;
        table_id_t table_id{0};
        std::vector<col_id_t> key_cols;   // 索引列（按顺序构成复合键）
        IndexType type{IndexType::BPlusTree};
        bool unique{false};
        page_id_t root_page_id{-1};       // 预留：磁盘版索引根页（内存版为 -1）

        std::shared_ptr<Index> index;     // 活的索引实例
    };

    // 索引定义的磁盘记录（定长，便于序列化到 catalog 索引页）。
    // 不含索引数据本身——重启后扫表重建。
    constexpr size_t INDEX_NAME_MAX_LEN = 48;
    constexpr size_t INDEX_MAX_KEY_COLS = 8;

    struct IndexDefRecord {
        uint32_t index_id{0};
        char index_name[INDEX_NAME_MAX_LEN]{};
        table_id_t table_id{0};
        uint8_t type{0};                  // IndexType
        uint8_t unique{0};
        uint8_t key_count{0};
        uint8_t reserved{0};
        col_id_t key_cols[INDEX_MAX_KEY_COLS]{};
        int64_t root_page_id{-1};         // 磁盘版 B+树根页（内存版索引为 -1）
    };
}
