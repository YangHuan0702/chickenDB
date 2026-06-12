//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_create_statement.h"
#include "binder/statement/bound_create_index_statement.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_create_table.h"
#include "planner/logical/logical_create_index.h"

using namespace chickenDB;

auto Planner::LogicalCreatePlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::CREATE,"[Logical] create table planner error, statement type not is create.");
    auto bound_create_statement = dynamic_cast<BoundCreateStatement*>(bound_statement.get());

    auto logical_create_table = std::make_unique<LogicalCreateTable>();
    logical_create_table->table_name_ = bound_create_statement->table_name_;
    logical_create_table->columns_ = std::move(bound_create_statement->columns_);
    return logical_create_table;
}

auto Planner::LogicalCreateIndexPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::CREATE_INDEX,
                                      "[Logical] statement type is not create index.");
    auto *bound = dynamic_cast<BoundCreateIndexStatement *>(bound_statement.get());

    auto logical = std::make_unique<LogicalCreateIndex>();
    logical->index_name_ = bound->index_name_;
    logical->table_id_ = bound->table_id_;
    logical->key_cols_ = bound->key_cols_;
    logical->unique_ = bound->unique_;
    return logical;
}

