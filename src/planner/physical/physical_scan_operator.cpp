//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_scan.h"
#include "planner/physical/scan/physical_seq_scan.h"
using namespace chickenDB;

auto Planner::PhysicalScanOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::SCAN,
                                      "[Planner] Physical Operator handler error,target logical operator is not Scan type.");
    auto logical_scan = dynamic_cast<LogicalScan*>(logical_operator.get());
    return std::make_unique<PhysicalSeqScan>(logical_scan->table_id_,catalog_);
}

