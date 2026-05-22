//
// Created by huan.yang on 2026-05-22.
//
#include "common/chicken_execption.h"
#include "executor/execution.h"
#include "planner/physical/ddl/physical_create.h"

using namespace chickenDB;

auto Execution::ExecuteCreate(std::unique_ptr<PhysicalOperator> plan) -> void {
    ChickenException::AssertCondition(plan->type_ == PhysicalOperatorType::CREATE_TABLE,
                                      "[ExecuteCreate] target physical operator is not create_table type.");
    auto physical_create_table = dynamic_cast<PhysicalCreateTable *>(plan.get());

    context_->catalog_->CreateTable(physical_create_table->table_name_, physical_create_table->columns_,
                                    physical_create_table->create_ts_);
}
