//
// Created by huan.yang on 2026-05-21.
//
#include "planner/planner.h"
using namespace chickenDB;

auto Planner::PhysicalProjectOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator> {
    return nullptr;
}
