//
// Created by huan.yang on 2026-05-25.
//
#pragma once
#include <vector>

#include "buffer_manager.h"
#include "page.h"
#include "page_codec.h"
#include "catalog/schema_version.h"
#include "common/constants.h"
#include "common/enum/compression_type.h"
#include "common/enum/page_type.h"

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
    │         Null Bitmap Section         │  ← 每列一个 bitmap，列序排列
    ├─────────────────────────────────────┤
    │          Page Checksum (8B)         │  ← 末尾校验（v1 写 0）
    └─────────────────────────────────────┘  ← offset PAGE_SIZE
     */
    struct ColDirEntry {
        col_id_t col_id; // 列 ID
        uint32_t data_offset; // 在 page 内的起始位置（绝对偏移）
        uint32_t compressed_size; // 压缩后大小
        uint32_t raw_size; // 解压后大小（用于预分配/解压）
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

    // 一列待写入的原始（未压缩）定长数据 + 可选 null bitmap。
    // data 指向 num_rows * TypeSize(type) 字节；validity 为 (num_rows+7)/8 字节，
    // 传 nullptr 表示该列全部非空。
    struct ColumnInput {
        col_id_t col_id;
        ColumnType type;
        const char *data;
        uint32_t raw_size;
        const uint8_t *validity;
    };

    class TableDataPage {
    public:
        // 序列化时各区段的固定布局常量。
        static constexpr uint16_t K_HEADER_SIZE = 64;
        static constexpr uint32_t K_COL_DIR_ENTRY_SIZE = 16;

        explicit TableDataPage(table_id_t table_id, Page *page) : table_id_(table_id), header_(),
                                                                  page_(page) {
            Init();
        }

        ~TableDataPage() {
        }

        // ---- 写入 ----
        // 把若干列的原始数据按列式 + 指定压缩算法序列化进 page。
        // 成功返回 true；放不下（超 PAGE_SIZE）返回 false。
        auto BuildFromColumns(const std::vector<ColumnInput> &cols, uint32_t num_rows,
                              CompressionType compression, uint64_t base_row_id,
                              uint64_t create_ts) -> bool;

        // ---- 读取 accessor ----
        auto RowCount() const -> size_t;
        auto FreeSpace() const -> size_t;
        auto NumColumns() const -> uint16_t { return header_.num_columns; }
        auto NumRows() const -> uint32_t { return header_.num_rows; }
        auto Compression() const -> CompressionType {
            return static_cast<CompressionType>(header_.compression);
        }

        // 是否为合法数据页（魔数 + 页类型校验），扫描时用于跳过空页/非数据页。
        auto IsDataPage() const -> bool {
            return header_.magic == static_cast<uint32_t>(PAGE_MAGIC_NUM) &&
                   header_.page_type == static_cast<uint8_t>(PageType::DATA);
        }

        auto GetColumnDir(size_t col_idx) const -> const ColDirEntry & { return cols_.at(col_idx); }

        // 解压第 col_idx 列到 out，返回 raw_size。
        auto GetColumnRaw(size_t col_idx, std::vector<char> &out) const -> size_t;

        // 拷贝第 col_idx 列的 null bitmap 到 out（(num_rows+7)/8 字节）。
        auto GetColumnValidity(size_t col_idx, std::vector<uint8_t> &out) const -> void;

        table_id_t table_id_;

    private:
        auto Init() -> void;
        auto InitHeader() -> void;
        auto InitCols() -> void;
        auto WriteHeader() -> void;

        PageHeader header_;
        std::vector<ColDirEntry> cols_{};
        Page *page_;
        // page_id_t last_page_no_{0};
        bool inited{false};
    };
}
