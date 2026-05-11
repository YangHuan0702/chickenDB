//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_update_statement.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_update.h"

using namespace chickenDB;

auto Planner::LogicalUpdatePlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::UPDATE,
                                      "[Logical] create table planner error, statement type not is update.");

    auto bound_update_statement = dynamic_cast<BoundUpdateStatement*>(bound_statement.get());

    auto table_catalog_entry = catalog_->GetTable(bound_update_statement->table_id_);

    auto logical_update = std::make_unique<LogicalUpdate>(table_catalog_entry);
    logical_update->col_ids_ = std::move(bound_update_statement->col_ids_);
    logical_update->values_ = std::move(bound_update_statement->values_);
    logical_update->children_.push_back(LogicalOperatorForExpression(std::move(bound_update_statement->where_)));
    return logical_update;
}
