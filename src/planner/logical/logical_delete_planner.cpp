//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_delete_statement.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_delete.h"
#include "sql/DeleteStatement.h"

using namespace chickenDB;


auto Planner::LogicalDeletePlanner(
    std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::DELETE,
                                      "[Logical] create table planner error, statement type not is delete.");

    auto delete_statement = dynamic_cast<BoundDeleteStatement*>(bound_statement.get());

    const TableCatalogEntry *table = catalog_->GetTable(delete_statement->table_id_);

    auto logical_delete = std::make_unique<LogicalDelete>(table);

    logical_delete->children_.push_back(LogicalOperatorForExpression(std::move(delete_statement->where_)));

    return logical_delete;
}
