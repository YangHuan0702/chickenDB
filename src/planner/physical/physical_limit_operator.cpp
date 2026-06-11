//
// Created by huan.yang on 2026-05-21.
//
#include "planner/planner.h"
#include "planner/logical/logical_limit.h"
#include "planner/physical/physical_limit.h"
#include "executor/chunk_util.h"
#include "common/chicken_execption.h"

using namespace chickenDB;

auto Planner::PhysicalLimitOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::LIMIT,
                                      "[Planner] target logical operator is not Limit type.");
    auto *logical_limit = dynamic_cast<LogicalLimit *>(logical_operator.get());
    return std::make_unique<PhysicalLimit>(static_cast<size_t>(logical_limit->start_),
                                           static_cast<size_t>(logical_limit->size_));
}


auto PhysicalLimit::Init() -> void {
    Child(0)->Init();
    seen_ = 0;
    emitted_ = 0;
}

auto PhysicalLimit::Close() -> void {
    Child(0)->Close();
}

// 跨 chunk 跟踪：跳过前 start_ 行，最多输出 offset_ 行。把当前 chunk 中落入
// [start_, start_+offset_) 窗口的行物化到 output_。
auto PhysicalLimit::Next() -> Chunk * {
    if (emitted_ >= offset_) {
        return nullptr;
    }

    while (Chunk *in = Child(0)->Next()) {
        const size_t n = in->Count();
        output_.Init(ChunkUtil::TypesOf(*in), n == 0 ? 1 : n);
        output_.SetColIds(in->ColIds());

        size_t out_row = 0;
        for (size_t r = 0; r < n; r++) {
            const size_t global_idx = seen_ + r;
            if (global_idx < start_) {
                continue; // 还在 offset 窗口之前
            }
            if (emitted_ >= offset_) {
                break; // 已满
            }
            ChunkUtil::CopyRow(output_, out_row, *in, r);
            out_row++;
            emitted_++;
        }
        seen_ += n;

        if (out_row > 0) {
            output_.SetCount(out_row);
            return &output_;
        }
        if (emitted_ >= offset_) {
            return nullptr;
        }
    }
    return nullptr;
}
