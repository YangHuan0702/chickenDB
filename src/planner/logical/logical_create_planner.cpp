//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_create_statement.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_create_table.h"

using namespace chickenDB;

auto Planner::LogicalCreatePlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::CREATE,"[Logical] create table planner error, statement type not is create.");
    auto bound_create_statement = dynamic_cast<BoundCreateStatement*>(bound_statement.get());

    auto logical_create_table = std::make_unique<LogicalCreateTable>();
    logical_create_table->table_name_ = bound_create_statement->table_name_;
    logical_create_table->columns_ = std::move(bound_create_statement->columns_);
    return logical_create_table;
}

