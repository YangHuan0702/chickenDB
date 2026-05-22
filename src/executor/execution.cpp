//
// Created by huan.yang on 2026-05-21.
//
#include "executor/execution.h"

using namespace chickenDB;


auto Execution::Exec(std::unique_ptr<PhysicalOperator> plan) -> void {
    switch (plan->type_) {
        case PhysicalOperatorType::CREATE_TABLE: ExecuteCreate(std::move(plan));
        default: throw std::invalid_argument("Invalid PhysicalOperator type");
    }
}
