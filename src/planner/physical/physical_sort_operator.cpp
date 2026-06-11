//
// Created by huan.yang on 2026-05-21.
//
#include <algorithm>
#include <vector>

#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_sort.h"
#include "planner/physical/sort/physical_in_memory_sort.h"
#include "planner/physical/sort/physical_top_n.h"
#include "planner/physical/sort/physical_external_sort.h"
#include "executor/chunk_util.h"

using namespace chickenDB;

namespace {
    // 把孩子的全部输出物化为 rows（每行各列值取 double），并记录列类型/col_ids。
    auto Materialize(PhysicalOperator *child, std::vector<std::vector<double>> &rows,
                     std::vector<ColumnType> &types, std::vector<col_id_t> &col_ids,
                     std::vector<size_t> &sort_idx, const std::vector<col_id_t> &sort_cols) -> void {
        bool resolved = false;
        while (Chunk *in = child->Next()) {
            if (!resolved) {
                types = ChunkUtil::TypesOf(*in);
                col_ids = in->ColIds();
                auto col_map = ChunkUtil::BuildColMap(*in);
                for (col_id_t cid : sort_cols) {
                    auto it = col_map.find(cid);
                    ChickenException::AssertCondition(it != col_map.end(), "[Sort] sort column not found");
                    sort_idx.push_back(it->second);
                }
                resolved = true;
            }
            const size_t n = in->Count();
            const size_t cols = in->ColumnCount();
            for (size_t r = 0; r < n; r++) {
                std::vector<double> row(cols);
                for (size_t c = 0; c < cols; c++) {
                    const Vector &v = in->GetColumn(c);
                    row[c] = v.GetType() == ColumnType::NUMBER
                                 ? static_cast<double>(v.GetValue<int32_t>(r))
                                 : v.GetValue<double>(r);
                }
                rows.push_back(std::move(row));
            }
        }
    }

    // 按 sort_idx 列升序比较两行。
    auto MakeComparator(const std::vector<size_t> &sort_idx) {
        return [sort_idx](const std::vector<double> &a, const std::vector<double> &b) {
            for (size_t idx : sort_idx) {
                if (a[idx] < b[idx]) return true;
                if (a[idx] > b[idx]) return false;
            }
            return false;
        };
    }

    // 把已排序的 rows 物化进 output（一次性，全部行）。
    auto EmitRows(Chunk &output, const std::vector<std::vector<double>> &rows,
                  const std::vector<ColumnType> &types, const std::vector<col_id_t> &col_ids) -> void {
        const size_t n = rows.empty() ? 1 : rows.size();
        output.Init(types, n);
        output.SetColIds(col_ids);
        for (size_t r = 0; r < rows.size(); r++) {
            for (size_t c = 0; c < types.size(); c++) {
                if (types[c] == ColumnType::NUMBER) {
                    output.GetColumn(c).SetValue<int32_t>(r, static_cast<int32_t>(rows[r][c]));
                } else {
                    output.GetColumn(c).SetValue<double>(r, rows[r][c]);
                }
            }
        }
        output.SetCount(rows.size());
    }
}

auto Planner::PhysicalSortOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::SORT,
                                      "[Planner] target logical operator is not Sort type.");
    auto *logical_sort = dynamic_cast<LogicalSort *>(logical_operator.get());
    // 默认选内存排序；外部排序/TopN 由优化器按代价/LIMIT 选择（后续）。
    return std::make_unique<PhysicalInMemorySort>(logical_sort->col_ids_);
}


// ---- InMemorySort ----
auto PhysicalInMemorySort::Init() -> void {
    Child(0)->Init();
    rows_.clear();
    emit_pos_ = 0;
    built_ = false;
}

auto PhysicalInMemorySort::Close() -> void {
    Child(0)->Close();
}

auto PhysicalInMemorySort::Next() -> Chunk * {
    if (!built_) {
        std::vector<size_t> sort_idx;
        Materialize(Child(0), rows_, types_, col_ids_, sort_idx, sort_cols_);
        std::sort(rows_.begin(), rows_.end(), MakeComparator(sort_idx));
        EmitRows(output_, rows_, types_, col_ids_);
        built_ = true;
        return rows_.empty() ? nullptr : &output_;
    }
    return nullptr;
}


// ---- TopN ----
auto PhysicalTopN::Init() -> void {
    Child(0)->Init();
    rows_.clear();
    built_ = false;
}

auto PhysicalTopN::Close() -> void {
    Child(0)->Close();
}

auto PhysicalTopN::Next() -> Chunk * {
    if (!built_) {
        std::vector<size_t> sort_idx;
        Materialize(Child(0), rows_, types_, col_ids_, sort_idx, sort_cols_);
        std::sort(rows_.begin(), rows_.end(), MakeComparator(sort_idx));
        if (rows_.size() > n_) {
            rows_.resize(n_);
        }
        EmitRows(output_, rows_, types_, col_ids_);
        built_ = true;
        return rows_.empty() ? nullptr : &output_;
    }
    return nullptr;
}


// ---- ExternalSort（内存回退） ----
auto PhysicalExternalSort::Init() -> void {
    Child(0)->Init();
    rows_.clear();
    emit_pos_ = 0;
    built_ = false;
}

auto PhysicalExternalSort::Close() -> void {
    Child(0)->Close();
}

auto PhysicalExternalSort::Next() -> Chunk * {
    if (!built_) {
        std::vector<size_t> sort_idx;
        Materialize(Child(0), rows_, types_, col_ids_, sort_idx, sort_cols_);
        std::sort(rows_.begin(), rows_.end(), MakeComparator(sort_idx));
        EmitRows(output_, rows_, types_, col_ids_);
        built_ = true;
        return rows_.empty() ? nullptr : &output_;
    }
    return nullptr;
}
