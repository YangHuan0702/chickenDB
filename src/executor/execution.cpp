//
// Created by huan.yang on 2026-05-21.
//
#include "executor/execution.h"

#include "executor/executor_context.h"
#include "planner/physical/physical_operator.h"

using namespace chickenDB;


auto Execution::Exec(std::unique_ptr<PhysicalOperator> plan) -> void {
    switch (plan->type_) {
        case PhysicalOperatorType::CREATE_TABLE:
            ExecuteCreate(std::move(plan));
            break;
        case PhysicalOperatorType::INSERT:
            ExecuteInsert(std::move(plan));
            break;
        case PhysicalOperatorType::CREATE_INDEX:
            ExecuteCreateIndex(std::move(plan));
            break;
        case PhysicalOperatorType::DELETE:
            ExecuteDelete(std::move(plan));
            break;
        case PhysicalOperatorType::UPDATE:
            ExecuteUpdate(std::move(plan));
            break;
        default:
            // 其余（SeqScan/Filter/Project/Agg/Sort/Join 等）都是以 Chunk 流为输出的
            // 查询计划，走统一的 volcano 拉取循环。
            ExecuteQuery(std::move(plan));
            break;
    }
}

auto Execution::AttachContext(PhysicalOperator *op) -> void {
    if (op == nullptr) return;
    op->ctx_ = context_.get();
    for (size_t i = 0; i < op->ChildCount(); i++) {
        AttachContext(op->Child(i));
    }
}

auto Execution::ExecuteQuery(std::unique_ptr<PhysicalOperator> plan) -> void {
    result_rows_.clear();
    AttachContext(plan.get());

    plan->Init();
    while (Chunk *chunk = plan->Next()) {
        const size_t n = chunk->Count();
        const size_t cols = chunk->ColumnCount();
        for (size_t r = 0; r < n; r++) {
            std::vector<Value> row;
            row.reserve(cols);
            for (size_t c = 0; c < cols; c++) {
                const Vector &vec = chunk->GetColumn(c);
                if (!vec.IsValid(r)) {
                    row.emplace_back(Value(std::monostate{}));
                    continue;
                }
                switch (vec.GetType()) {
                    case ColumnType::NUMBER:
                        row.emplace_back(Value(vec.GetValue<int32_t>(r)));
                        break;
                    case ColumnType::DOUBLE:
                        row.emplace_back(Value(vec.GetValue<double>(r)));
                        break;
                    default:
                        row.emplace_back(Value(std::monostate{}));
                        break;
                }
            }
            result_rows_.push_back(std::move(row));
        }
    }
    plan->Close();
}
