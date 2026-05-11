//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_insert_statement.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_insert.h"

using namespace chickenDB;


auto Planner::LogicalInsertPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::INSERT,
                                      "[Logical] create table planner error, statement type not is insert.");

    auto bound_insert_statement = dynamic_cast<BoundInsertStatement*>(bound_statement.get());

    auto table_catalog_entry = catalog_->GetTable(bound_insert_statement->table_id_);

    auto logical_insert = std::make_unique<LogicalInsert>(table_catalog_entry);
    logical_insert->col_ids_ = std::move(bound_insert_statement->col_ids_);
    logical_insert->values_ = std::move(bound_insert_statement->values_);
    return logical_insert;
}
