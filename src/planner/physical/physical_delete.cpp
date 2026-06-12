//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_delete.h"
#include "planner/logical/logical_filter.h"
#include "planner/physical/dml/physical_delete.h"

using namespace chickenDB;

auto Planner::PhysicalDelete(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::DELETE,
                                      "[Planner] target logical operator is not Delete type.");
    auto *logical_delete = dynamic_cast<LogicalDelete *>(logical_operator.get());
    ChickenException::AssertCondition(logical_delete->table_ != nullptr, "[Planner] delete null table");

    // 从 LogicalFilter 子节点取出谓词（DELETE 的 where）。
    std::unique_ptr<BoundExpression> predicate;
    if (!logical_delete->children_.empty()) {
        auto *filter = dynamic_cast<LogicalFilter *>(logical_delete->children_[0].get());
        if (filter != nullptr) {
            predicate = std::move(filter->condition_);
        }
    }
    return std::make_unique<PhysicalDelete>(logical_delete->table_->table_id, std::move(predicate));
}

auto PhysicalDelete::Init() -> void {}
auto PhysicalDelete::Next() -> Chunk * { return nullptr; }
auto PhysicalDelete::Close() -> void {}
