//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_create_table.h"
#include "planner/physical/ddl/physical_create.h"

using namespace chickenDB;

auto Planner::PhysicalCreateTable(
    std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::CREATEA_TABLE,
                                      "[Planner] Physical Operator handler error,target logical operator is not CerateTable type.");
    auto logical_create_table = dynamic_cast<LogicalCreateTable*>(logical_operator.get());

    return std::make_unique<chickenDB::PhysicalCreateTable>(logical_create_table->table_name_,logical_create_table->columns_);
}


auto PhysicalCreateTable::Close() -> void {

}

auto PhysicalCreateTable::Init() -> void {

}

auto PhysicalCreateTable::Next() -> Chunk * {
    return nullptr;
}



