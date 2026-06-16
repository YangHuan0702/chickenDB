//
// Created by huan.yang on 2026-06-11.
//
#include "buffer/table_heap.h"

#include <cstring>
#include <vector>

#include "buffer/table_data_page.h"
#include "executor/varchar_codec.h"

using namespace chickenDB;

// 解码 rid 所在数据页，把第 rid.row_idx 行各列值拷进 out 的第 out_row 行。
// 实现上整页解压各列（与扫描一致），再按行下标取值。
auto TableHeap::FetchRow(const RID &rid, Chunk &out, size_t out_row) -> bool {
    Page *page = buffer_manager_->FetchPage(table_id_, rid.page_no);
    if (page == nullptr) {
        return false;
    }
    TableDataPage data_page(table_id_, page);
    if (!data_page.IsDataPage() || rid.row_idx >= data_page.NumRows()) {
        buffer_manager_->UnpinPage(table_id_, rid.page_no, false);
        return false;
    }

    const uint16_t num_cols = data_page.NumColumns();
    std::vector<char> raw;
    std::vector<uint8_t> validity;
    const size_t col_count = num_cols < out.ColumnCount() ? num_cols : out.ColumnCount();
    for (size_t c = 0; c < col_count; c++) {
        const size_t raw_size = data_page.GetColumnRaw(c, raw);
        Vector &vec = out.GetColumn(c);
        if (vec.IsVar()) {
            // 变长列：整列反序列化到临时 Vector，取目标行追加到 out（out_row 顺序递增，
            // 与 AppendString 的顺序语义一致）。
            std::string_view sv;
            Vector tmp;
            if (raw_size > 0) {
                tmp.Init(vec.GetType(), static_cast<size_t>(data_page.NumRows()));
                VarcharCodec::DeserializeVarlenColumn(raw.data(), raw_size, tmp);
                if (rid.row_idx < tmp.VarRowCount()) {
                    sv = tmp.GetString(rid.row_idx);
                }
            }
            vec.AppendString(sv);
        } else {
            const size_t ts = vec.TypeSize();
            // 该列在页内第 row_idx 个值的偏移。
            const size_t off = static_cast<size_t>(rid.row_idx) * ts;
            if (off + ts <= raw_size) {
                std::memcpy(vec.GetData() + out_row * ts, raw.data() + off, ts);
            }
        }
        data_page.GetColumnValidity(c, validity);
        bool valid = (validity[rid.row_idx / 8] >> (rid.row_idx % 8)) & 1U;
        vec.SetValidity(out_row, valid);
    }

    buffer_manager_->UnpinPage(table_id_, rid.page_no, false);
    return true;
}
