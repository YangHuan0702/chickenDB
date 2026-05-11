//
// Created by huan.yang on 2026-05-11.
//
#include "binder/expression/bound_column_expression.h"
#include "planner/planner.h"
#include "planner/logical/logical_project.h"

using namespace chickenDB;

auto Planner::LogicalOperatorProject(const std::vector<std::unique_ptr<BoundExpression>>& statement) -> std::unique_ptr<LogicalOperator> {

    auto logical_project = std::make_unique<LogicalProject>();
    for (const auto &bound_expression : statement) {
        auto bound_column_expression = dynamic_cast<BoundColumnExpression*>(bound_expression.get());
        logical_project->col_ids_.push_back(bound_column_expression->col_id_);
    }
    return logical_project;
}

