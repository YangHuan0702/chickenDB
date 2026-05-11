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
