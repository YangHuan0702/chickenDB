//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_insert.h"
#include "planner/physical/dml/physical_insert.h"

using namespace chickenDB;

auto Planner::PhysicalInsert(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::INSERT,
                                      "[Planner] Physical Operator handler error, target logical operator is not Insert type.");
    auto logical_insert = dynamic_cast<LogicalInsert *>(logical_operator.get());

    return std::make_unique<chickenDB::PhysicalInsert>(logical_insert->table_catalog_entry_,
                                                       std::move(logical_insert->col_ids_),
                                                       std::move(logical_insert->values_));
}


auto PhysicalInsert::Init() -> void {
}

auto PhysicalInsert::Next() -> Chunk * {
    return nullptr;
}

auto PhysicalInsert::Close() -> void {
}
