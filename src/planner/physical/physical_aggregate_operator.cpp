//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
using namespace chickenDB;

auto Planner::PhysicalAggregateOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::AGGREGATE,"[Planner] Physical Aggregate Operator handler error,target logical operator is not Aggregate type.");

    // std::make_unique<PhysicalFilter()
    return nullptr;
}
