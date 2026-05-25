//
// Created by huan.yang on 2026-05-25.
//
#pragma once
#include <memory>

#include "buffer_manager.h"
#include "page.h"
#include "catalog/schema_version.h"

namespace chickenDB {
    /**
    ┌─────────────────────────────────────┐  ← offset 0
    │           Page Header (64B)         │
    ├─────────────────────────────────────┤
    │        Column Directory             │  ← 每列的 offset/length 索引
    │        (N × 16B per column)         │
    ├─────────────────────────────────────┤
    │                                     │
    │         Column Data Blocks          │  ← 各列压缩数据，紧密排列
    │   [Col0][Col1][Col2]...[ColN]       │
    │                                     │
    ├─────────────────────────────────────┤
    │         Null Bitmap Section         │  ← 每列一个 bitmap，可选
    ├─────────────────────────────────────┤
    │          Page Checksum (8B)         │  ← 末尾校验
    └─────────────────────────────────────┘  ← offset PAGE_SIZE (默认 64KB 或 256KB)
     */
    struct ColDirEntry {
        col_id_t col_id; // 列 ID
        uint32_t data_offset; // 在 page 内的起始位置
        uint32_t compressed_size; // 压缩后大小
        uint32_t raw_size; // 解压后大小（用于预分配）
    };

    struct PageHeader {
        uint32_t magic; // 魔数，校验 page 类型
        uint16_t version; // 格式版本
        uint8_t page_type; // DATA / INDEX / META
        uint8_t compression; // NONE / ZSTD / LZ4 / SNAPPY
        page_id_t page_id; // 全局唯一 page id
        uint32_t num_rows; // 本 page 行数
        uint16_t num_columns; // 列数
        uint16_t col_dir_offset; // Column Directory 起始 offset
        uint32_t data_offset; // Column Data 起始 offset
        uint32_t null_bitmap_offset;
        uint64_t min_row_id; // 行 ID 范围（用于过滤）
        uint64_t max_row_id;
        uint64_t create_ts; // 写入时间戳
        uint8_t reserved[4];
    };


    class TableDataPage {
    public:
        explicit TableDataPage(table_id_t table_id, Page *page) : table_id_(table_id), header_(),
                                                                  page_(page) {
            Init();
        }

        ~TableDataPage() {
        }


        auto RowCount() const -> size_t;

        auto FreeSpace() const -> size_t;

        table_id_t table_id_;

    private:
        auto Init() -> void;

        auto InitHeader() -> void;

        auto InitCols() -> void;

        PageHeader header_;
        std::vector<ColDirEntry> cols_{};
        Page *page_;
        page_id_t last_page_no_{0};
        bool inited{false};
    };
}
