//
// Created by huan.yang on 2026-06-11.
//
#include <vector>

#include "common/chicken_execption.h"
#include "executor/execution.h"
#include "executor/executor_context.h"
#include "buffer/table_data_page.h"
#include "planner/physical/dml/physical_insert.h"
#include "transaction/log_record.h"

using namespace chickenDB;

namespace {
    // 把 variant 里的整数（解析器产出 int64_t，亦兼容 int）取成 int32。
    auto AsInt32(const Value &v) -> int32_t {
        if (std::holds_alternative<int64_t>(v.value_)) {
            return static_cast<int32_t>(std::get<int64_t>(v.value_));
        }
        if (std::holds_alternative<int>(v.value_)) {
            return static_cast<int32_t>(std::get<int>(v.value_));
        }
        throw ChickenException("[ExecuteInsert] value is not an integer for NUMBER column");
    }

    // 把 variant 里的浮点（解析器产出 double，亦兼容 float）取成 double。
    auto AsDouble(const Value &v) -> double {
        if (std::holds_alternative<double>(v.value_)) {
            return std::get<double>(v.value_);
        }
        if (std::holds_alternative<float>(v.value_)) {
            return static_cast<double>(std::get<float>(v.value_));
        }
        if (std::holds_alternative<int64_t>(v.value_)) {
            return static_cast<double>(std::get<int64_t>(v.value_));
        }
        throw ChickenException("[ExecuteInsert] value is not numeric for DOUBLE column");
    }
}

auto Execution::ExecuteInsert(std::unique_ptr<PhysicalOperator> plan) -> void {
    ChickenException::AssertCondition(plan->type_ == PhysicalOperatorType::INSERT,
                                      "[ExecuteInsert] target physical operator is not insert type.");
    auto *insert = dynamic_cast<PhysicalInsert *>(plan.get());
    ChickenException::AssertCondition(insert != nullptr, "[ExecuteInsert] dynamic_cast failed");
    ChickenException::AssertCondition(insert->table_ != nullptr, "[ExecuteInsert] null table entry");

    const table_id_t table_id = insert->table_->table_id;
    const SchemaPage *schema = context_->catalog_->GetSchema(table_id);
    ChickenException::AssertCondition(schema != nullptr, "[ExecuteInsert] schema not found");

    const auto &schema_cols = schema->columns_;
    const size_t num_specified = insert->col_ids_.size();
    ChickenException::AssertCondition(num_specified > 0, "[ExecuteInsert] no target columns");
    ChickenException::AssertCondition(insert->values_.size() % num_specified == 0,
                                      "[ExecuteInsert] values count not a multiple of column count");
    const uint32_t num_rows = static_cast<uint32_t>(insert->values_.size() / num_specified);
    ChickenException::AssertCondition(num_rows > 0, "[ExecuteInsert] no rows to insert");

    // 建立 col_id -> 在本次 INSERT values 行内的列位置。
    std::unordered_map<col_id_t, size_t> col_id_to_pos;
    for (size_t j = 0; j < num_specified; j++) {
        col_id_to_pos[insert->col_ids_[j]] = j;
    }

    // 为每个 schema 列（仅定长类型）构造列式 raw buffer + null bitmap。
    const size_t bitmap_bytes = (num_rows + 7) / 8;
    std::vector<std::vector<char>> col_buffers;
    std::vector<std::vector<uint8_t>> col_validity;
    col_buffers.reserve(schema_cols.size());
    col_validity.reserve(schema_cols.size());

    for (const auto &col : schema_cols) {
        const size_t type_size = TypeSizeConversion::TypeSize(col.data_type);
        ChickenException::AssertCondition(type_size > 0,
                                          "[ExecuteInsert] unsupported (variable-length) column type");

        std::vector<char> buf(static_cast<size_t>(num_rows) * type_size, 0);
        std::vector<uint8_t> validity(bitmap_bytes, 0xFF);

        auto pos_it = col_id_to_pos.find(col.col_id);
        for (uint32_t r = 0; r < num_rows; r++) {
            char *slot = buf.data() + static_cast<size_t>(r) * type_size;
            if (pos_it == col_id_to_pos.end()) {
                // 该列未在 INSERT 中指定 -> NULL。
                validity[r / 8] &= static_cast<uint8_t>(~(1U << (r % 8)));
                continue;
            }
            const Value &v = insert->values_[static_cast<size_t>(r) * num_specified + pos_it->second];
            if (std::holds_alternative<std::monostate>(v.value_)) {
                validity[r / 8] &= static_cast<uint8_t>(~(1U << (r % 8)));
                continue;
            }
            if (col.data_type == ColumnType::NUMBER) {
                int32_t iv = AsInt32(v);
                std::memcpy(slot, &iv, sizeof(int32_t));
            } else if (col.data_type == ColumnType::DOUBLE) {
                double dv = AsDouble(v);
                std::memcpy(slot, &dv, sizeof(double));
            } else {
                throw ChickenException("[ExecuteInsert] unsupported fixed type");
            }
        }

        col_buffers.push_back(std::move(buf));
        col_validity.push_back(std::move(validity));
    }

    // 多页分裂：从 row_start 起尽量多地塞进一页，放不下就减半重试，逐页写入。
    const size_t ncols = schema_cols.size();
    const txn_id_t tid = context_->HasTxn() ? context_->txn_->GetTxnId() : INVALID_TXN_ID;
    uint32_t row_start = 0;
    while (row_start < num_rows) {
        uint32_t cnt = num_rows - row_start; // 本页尝试写入的行数
        Page *page = context_->buffer_manager_->NewPage(table_id);
        const page_id_t page_no = page->page_id_.page_no;

        // 为本页切片构造 ColumnInput（指向各列在 [row_start, row_start+cnt) 的子段）。
        // 注意 null bitmap 是按行 bit，切片需重算；这里 INSERT 全有效，给 nullptr 即全有效。
        auto build_slice = [&](uint32_t slice_cnt) -> bool {
            std::vector<ColumnInput> slice;
            slice.reserve(ncols);
            for (size_t c = 0; c < ncols; c++) {
                const size_t ts = TypeSizeConversion::TypeSize(schema_cols[c].data_type);
                slice.push_back(ColumnInput{
                    schema_cols[c].col_id, schema_cols[c].data_type,
                    col_buffers[c].data() + static_cast<size_t>(row_start) * ts,
                    static_cast<uint32_t>(slice_cnt * ts), nullptr});
            }
            TableDataPage dp(table_id, page);
            return dp.BuildFromColumns(slice, slice_cnt, CompressionType::ZSTD,
                                       insert->table_->row_count + row_start,
                                       insert->table_->create_ts);
        };

        while (cnt > 0 && !build_slice(cnt)) {
            cnt /= 2; // 放不下：减半重试
        }
        ChickenException::AssertCondition(cnt > 0, "[ExecuteInsert] single row exceeds page size");
        context_->buffer_manager_->UnpinPage(table_id, page_no, /*is_dirty=*/true);

        // 本页 [row_start, row_start+cnt) 的事务版本/WAL + 索引维护。
        for (uint32_t k = 0; k < cnt; k++) {
            const uint32_t global_r = row_start + k;
            RID rid(page_no, k);
            if (context_->HasTxn()) {
                if (context_->log_manager_ != nullptr) {
                    LogRecord rec;
                    rec.type = LogRecordType::INSERT;
                    rec.txn_id = tid;
                    rec.table_id = table_id;
                    rec.rid_page = page_no;
                    rec.rid_row = k;
                    context_->log_manager_->Append(rec);
                }
                context_->version_store_->OnInsert(rid, tid);
                context_->txn_->AppendInsert(rid);
            }
            auto col_value = [&](col_id_t cid) -> double {
                for (size_t c = 0; c < ncols; c++) {
                    if (schema_cols[c].col_id != cid) continue;
                    const char *slot = col_buffers[c].data() +
                                       static_cast<size_t>(global_r) * TypeSizeConversion::TypeSize(schema_cols[c].data_type);
                    if (schema_cols[c].data_type == ColumnType::NUMBER) {
                        int32_t iv; std::memcpy(&iv, slot, sizeof(int32_t)); return static_cast<double>(iv);
                    }
                    double dv; std::memcpy(&dv, slot, sizeof(double)); return dv;
                }
                return 0;
            };
            context_->catalog_->MaintainIndexInsert(table_id, col_value, rid);
        }

        row_start += cnt;
    }

    if (context_->HasTxn() && context_->log_manager_ != nullptr) {
        context_->log_manager_->Flush();
    }

    context_->catalog_->AddRowCount(table_id, num_rows);
}
