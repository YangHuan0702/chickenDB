//
// Created by huan.yang on 2026-05-21.
//
#include "planner/planner.h"
#include "planner/logical/logical_project.h"
#include "planner/physical/physical_project.h"
#include "executor/chunk_util.h"
#include "common/chicken_execption.h"

using namespace chickenDB;

auto Planner::PhysicalProjectOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::PROJECT,
                                      "[Planner] target logical operator is not Project type.");
    auto *logical_project = dynamic_cast<LogicalProject *>(logical_operator.get());
    auto physical = std::make_unique<PhysicalProject>();
    physical->cols_ = logical_project->col_ids_;
    return physical;
}


auto PhysicalProject::Init() -> void {
    Child(0)->Init();
    src_idx_.clear();
}

auto PhysicalProject::Close() -> void {
    Child(0)->Close();
}

// 从孩子拉 chunk，按 cols_（col_id）选出列子集重组成新 chunk。
auto PhysicalProject::Next() -> Chunk * {
    Chunk *in = Child(0)->Next();
    if (in == nullptr) {
        return nullptr;
    }

    // 首批：把投影列的 col_id 解析为输入 chunk 中的列下标。
    if (src_idx_.empty()) {
        auto col_map = ChunkUtil::BuildColMap(*in);
        for (col_id_t cid : cols_) {
            auto it = col_map.find(cid);
            ChickenException::AssertCondition(it != col_map.end(),
                                              "[Project] projected column not found in input");
            src_idx_.push_back(it->second);
        }
    }

    const size_t n = in->Count();
    std::vector<ColumnType> out_types;
    out_types.reserve(src_idx_.size());
    for (size_t idx : src_idx_) {
        out_types.push_back(in->GetColumn(idx).GetType());
    }
    output_.Init(out_types, n == 0 ? 1 : n);
    output_.SetColIds(cols_);

    for (size_t r = 0; r < n; r++) {
        ChunkUtil::CopyRowProjected(output_, r, *in, r, src_idx_);
    }
    output_.SetCount(n);
    return &output_;
}
