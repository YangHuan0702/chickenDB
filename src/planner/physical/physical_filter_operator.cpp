//
// Created by huan.yang on 2026-05-21.
//
#include "planner/planner.h"
#include "planner/logical/logical_filter.h"
#include "planner/physical/physical_filter.h"
#include "executor/expression_evaluator.h"
#include "executor/chunk_util.h"
#include "common/chicken_execption.h"

using namespace chickenDB;

auto Planner::PhysicalFilterOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::FILTER,
                                      "[Planner] target logical operator is not Filter type.");
    auto *logical_filter = dynamic_cast<LogicalFilter *>(logical_operator.get());
    return std::make_unique<PhysicalFilter>(std::move(logical_filter->condition_));
}


auto PhysicalFilter::Init() -> void {
    Child(0)->Init();
}

auto PhysicalFilter::Close() -> void {
    Child(0)->Close();
}

// 从孩子拉 chunk，对每行求谓词，命中行物化进 output_ 后返回。
// 跳过整批都不命中的 chunk，直到拿到非空结果或孩子耗尽。
auto PhysicalFilter::Next() -> Chunk * {
    while (Chunk *in = Child(0)->Next()) {
        if (col_map_.empty()) {
            col_map_ = ChunkUtil::BuildColMap(*in);
        }
        const size_t n = in->Count();

        output_.Init(ChunkUtil::TypesOf(*in), n == 0 ? 1 : n);
        output_.SetColIds(in->ColIds());

        size_t out_row = 0;
        for (size_t r = 0; r < n; r++) {
            if (ExpressionEvaluator::EvalPredicate(expression_.get(), *in, r, col_map_)) {
                ChunkUtil::CopyRow(output_, out_row, *in, r);
                out_row++;
            }
        }
        if (out_row > 0) {
            output_.SetCount(out_row);
            return &output_;
        }
        // 整批被过滤掉，继续拉下一批。
    }
    return nullptr;
}
