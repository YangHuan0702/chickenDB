//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_scan.h"
#include "planner/physical/scan/physical_seq_scan.h"
#include "planner/physical/scan/physical_index_scan.h"
using namespace chickenDB;

auto Planner::PhysicalScanOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::SCAN,
                                      "[Planner] Physical Operator handler error,target logical operator is not Scan type.");
    auto logical_scan = dynamic_cast<LogicalScan*>(logical_operator.get());

    // 命中可用索引：生成索引点查扫描。
    if (logical_scan->use_index_) {
        auto index_scan = std::make_unique<PhysicalIndexScan>(logical_scan->table_id_, catalog_, nullptr);
        index_scan->index_name_ = logical_scan->index_name_;
        index_scan->lookup_lo_ = logical_scan->lookup_key_;
        index_scan->lookup_hi_ = logical_scan->lookup_key_;
        index_scan->is_range_ = false;
        return index_scan;
    }
    return std::make_unique<PhysicalSeqScan>(logical_scan->table_id_,catalog_);
}

