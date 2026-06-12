//
// Created by huan.yang on 2026-06-11.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_create_index.h"
#include "planner/physical/ddl/physical_create_index.h"

using namespace chickenDB;

auto Planner::PhysicalCreateIndex(std::unique_ptr<LogicalOperator> logical_operator)
    -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::CREATE_INDEX,
                                      "[Planner] target logical operator is not CreateIndex type.");
    auto *logical = dynamic_cast<LogicalCreateIndex *>(logical_operator.get());
    return std::make_unique<class  PhysicalCreateIndex>(logical->index_name_, logical->table_id_,
                                                 logical->key_cols_, logical->index_type_,
                                                 logical->unique_);
}

auto PhysicalCreateIndex::Init() -> void {}
auto PhysicalCreateIndex::Next() -> Chunk * { return nullptr; }
auto PhysicalCreateIndex::Close() -> void {}
