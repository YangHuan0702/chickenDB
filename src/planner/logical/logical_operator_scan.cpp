//
// Created by huan.yang on 2026-05-11.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_scan.h"

using namespace chickenDB;

auto Planner::LogicalOperatorScan(table_id_t table_id) -> std::unique_ptr<LogicalOperator> {
    auto table_catalog_entry = catalog_->GetTable(table_id);
    ChickenException::AssertCondition(nullptr != table_catalog_entry,"[Planner] logical operator scan error. unknown table.");

    return std::make_unique<LogicalScan>(table_id);
}
