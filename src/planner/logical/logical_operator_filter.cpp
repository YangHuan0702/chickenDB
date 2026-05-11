//
// Created by huan.yang on 2026-05-11.
//
#include "planner/planner.h"
#include "planner/logical/logical_filter.h"
using namespace chickenDB;

auto Planner::LogicalOperatorFilter(std::unique_ptr<BoundExpression> statement) -> std::unique_ptr<LogicalOperator> {
    return std::make_unique<LogicalFilter>(std::move(statement));
}

