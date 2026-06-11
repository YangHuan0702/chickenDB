//
// Created by huan.yang on 2026-06-11.
//
#include <vector>

#include "common/chicken_execption.h"
#include "executor/execution.h"
#include "buffer/table_data_page.h"
#include "planner/physical/dml/physical_insert.h"

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
    std::vector<ColumnInput> inputs;
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
        inputs.push_back(ColumnInput{col.col_id, col.data_type, nullptr,
                                     static_cast<uint32_t>(col_buffers.back().size()), nullptr});
    }
    // buffers 已 move 定位，回填指针（避免悬垂）。
    for (size_t i = 0; i < inputs.size(); i++) {
        inputs[i].data = col_buffers[i].data();
        inputs[i].validity = col_validity[i].data();
    }

    // 分配新数据页并写入。v1：一条 INSERT 的所有行写入单页，放不下报错。
    Page *page = context_->buffer_manager_->NewPage(table_id);
    const page_id_t page_no = page->page_id_.page_no;
    const uint64_t base_row_id = insert->table_->row_count;

    TableDataPage data_page(table_id, page);
    bool ok = data_page.BuildFromColumns(inputs, num_rows, CompressionType::ZSTD,
                                         base_row_id, insert->table_->create_ts);
    context_->buffer_manager_->UnpinPage(table_id, page_no, /*is_dirty=*/true);
    ChickenException::AssertCondition(ok, "[ExecuteInsert] rows do not fit in a single page (v1 limit)");

    context_->catalog_->AddRowCount(table_id, num_rows);
}
