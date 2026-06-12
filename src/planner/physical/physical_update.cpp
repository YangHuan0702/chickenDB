//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_update.h"
#include "planner/logical/logical_filter.h"
#include "planner/physical/dml/physical_update.h"

using namespace chickenDB;

auto Planner::PhysicalUpdate(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::UPDATE,
                                      "[Planner] target logical operator is not Update type.");
    auto *logical_update = dynamic_cast<LogicalUpdate *>(logical_operator.get());
    ChickenException::AssertCondition(logical_update->table_ != nullptr, "[Planner] update null table");

    std::unique_ptr<BoundExpression> predicate;
    if (!logical_update->children_.empty()) {
        auto *filter = dynamic_cast<LogicalFilter *>(logical_update->children_[0].get());
        if (filter != nullptr) {
            predicate = std::move(filter->condition_);
        }
    }
    return std::make_unique<PhysicalUpdate>(logical_update->table_->table_id,
                                            std::move(logical_update->col_ids_),
                                            std::move(logical_update->values_),
                                            std::move(predicate));
}

auto PhysicalUpdate::Init() -> void {}
auto PhysicalUpdate::Next() -> Chunk * { return nullptr; }
auto PhysicalUpdate::Close() -> void {}
