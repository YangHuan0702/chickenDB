//
// Created by huan.yang on 2026-05-11.
//
#include "planner/planner.h"

using namespace chickenDB;


auto Planner::CreateLogicalPlanner(
    std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    switch (bound_statement->type_) {
        case StatementType::SELECT: return LogicalSelectPlanner(std::move(bound_statement));
        case StatementType::INSERT: return LogicalInsertPlanner(std::move(bound_statement));
        case StatementType::DELETE: return LogicalDeletePlanner(std::move(bound_statement));
        case StatementType::UPDATE: return LogicalUpdatePlanner(std::move(bound_statement));
        case StatementType::CREATE: return LogicalCreatePlanner(std::move(bound_statement));
        default: throw std::invalid_argument("[Planner] Unknown statement type");
    }
}



auto Planner::CreatePhysicalPlanner(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    if (nullptr == logical_operator) {
        return nullptr;
    }

    std::unique_ptr<PhysicalOperator> physical_operator = nullptr;
    switch (logical_operator->type_) {
        case LogicalOperatorType::SCAN :{
            physical_operator = PhysicalScanOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::FILTER : {
            physical_operator = PhysicalFilterOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::PROJECT : {
            physical_operator = PhysicalProjectOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::JOIN : {
            physical_operator = PhysicalJoinOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::AGGREGATE : {
            physical_operator = PhysicalAggregateOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::SORT : {
            physical_operator = PhysicalSortOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::LIMIT : {
            physical_operator = PhysicalLimitOperator(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::CREATEA_TABLE : {
            physical_operator = PhysicalCreateTable(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::DELETE : {
            physical_operator = PhysicalDelete(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::UPDATE : {
            physical_operator = PhysicalUpdate(std::move(logical_operator));
            break;
        }
        case LogicalOperatorType::INSERT : {
            physical_operator = PhysicalInsert(std::move(logical_operator));
            break;
        }
        default: throw std::invalid_argument("[Planner] Create physical Unknown logical operator type");
    }


    if (nullptr == physical_operator) {
        throw std::invalid_argument("[Planner] Create physical operator null");
    }

    if (!logical_operator->children_.empty()) {
        for (auto &operator_ : logical_operator->children_) {
            physical_operator->children_ = CreatePhysicalPlanner(std::move(operator_));
        }
    }
    return physical_operator;
}
