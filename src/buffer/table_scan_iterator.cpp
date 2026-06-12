//
// Created by huan.yang on 2026-05-25.
//
#include "buffer/table_scan_iterator.h"

#include <algorithm>
#include <cstring>

#include "buffer/table_data_page.h"
#include "common/chicken_execption.h"

using namespace chickenDB;

// 加载当前页号对应的页并校验是否为合法数据页。
// 返回 true 表示当前页可用于填充 chunk；false 表示是空页/非数据页（应跳过）。
auto TableScanIterator::LoadCurrentPage() -> bool {
    current_page_ = buffer_manager_->FetchPage(table_id_, current_page_no_);
    if (current_page_ == nullptr) {
        return false;
    }
    TableDataPage probe(table_id_, current_page_);
    if (!probe.IsDataPage()) {
        buffer_manager_->UnpinPage(table_id_, current_page_no_, false);
        current_page_ = nullptr;
        return false;
    }
    return true;
}

// 推进到下一页号。越过 last_page_id_ 返回 false（扫描结束）。
auto TableScanIterator::AdvancePage() -> bool {
    current_page_no_++;
    return current_page_no_ <= last_page_id_;
}

// 把已加载的当前数据页解码进 chunk：逐列解压 -> 写入对应 Vector -> 拷贝 null bitmap。
// 返回填入的行数。
auto TableScanIterator::FillChunkFromPage(Chunk &chunk) -> size_t {
    TableDataPage page(table_id_, current_page_);
    const uint32_t num_rows = page.NumRows();
    const uint16_t num_cols = page.NumColumns();

    // 按 schema 列顺序初始化 chunk 的列类型。
    std::vector<ColumnType> types;
    std::vector<col_id_t> col_ids;
    types.reserve(schema_page_->columns_.size());
    col_ids.reserve(schema_page_->columns_.size());
    for (const auto &col : schema_page_->columns_) {
        types.push_back(col.data_type);
        col_ids.push_back(col.col_id);
    }
    chunk.Init(types, num_rows == 0 ? 1 : num_rows);
    chunk.SetColIds(col_ids);

    std::vector<char> raw;
    std::vector<uint8_t> validity;
    const size_t col_count = std::min<size_t>(num_cols, types.size());
    for (size_t c = 0; c < col_count; c++) {
        const size_t raw_size = page.GetColumnRaw(c, raw);
        Vector &vec = chunk.GetColumn(c);
        if (raw_size > 0) {
            std::memcpy(vec.GetData(), raw.data(), raw_size);
        }
        page.GetColumnValidity(c, validity);
        std::memcpy(vec.GetValidity(), validity.data(), validity.size());
    }

    chunk.SetCount(num_rows);
    return num_rows;
}

// 顺序扫描：每次返回一页数据填入 output。无更多数据页返回 false。
// 约定：一页 -> 一个 chunk（页行数 <= K_VECTOR_SIZE）。
auto TableScanIterator::Next(Chunk &output) -> bool {
    if (!started_) {
        current_page_no_ = first_page_id_;
        started_ = true;
    } else {
        if (!AdvancePage()) {
            return false;
        }
    }

    // 跳过空页/非数据页，直到找到一个数据页或越界。
    while (current_page_no_ <= last_page_id_) {
        if (LoadCurrentPage()) {
            FillChunkFromPage(output);
            buffer_manager_->UnpinPage(table_id_, current_page_no_, false);
            current_page_ = nullptr;
            return true;
        }
        if (!AdvancePage()) {
            return false;
        }
    }
    return false;
}
