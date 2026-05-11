//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_select_statement.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_filter.h"
#include "planner/logical/logical_project.h"

using namespace chickenDB;

auto Planner::LogicalSelectPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::SELECT,
                                      "[Logical] create table planner error, statement type not is select.");
    auto bound_select_statement = dynamic_cast<BoundSelectStatement*>(bound_statement.get());

    // scan -> filter -> project

    auto root = LogicalOperatorScan(bound_select_statement->table_id_);

    if (bound_select_statement->where_) {
        auto filter = LogicalOperatorFilter(std::move(bound_select_statement->where_));
        filter->children_.push_back(std::move(root));
        root = std::move(filter);
    }

    auto project = LogicalOperatorProject(bound_select_statement->columns_);
    project->children_.push_back(std::move(root));
    root = std::move(project);
    return root;
}
