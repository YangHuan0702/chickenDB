//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>

#include "binder/expression/bound_expression.h"
#include "binder/statement/bound_statement.h"
#include "catalog/catalog.h"
#include "logical/logical_operator.h"
#include "physical/physical_operator.h"

namespace chickenDB {

    class Planner {
    public:
        explicit Planner() = default;
        ~Planner() = default;

        auto SetCatalog(std::shared_ptr<Catalog> catalog) -> void {
            catalog_ = std::move(catalog);
        }



        auto CreateLogicalPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalSelectPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalDeletePlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalUpdatePlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalInsertPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalCreatePlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalCreateIndexPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator>;


        auto CreatePhysicalPlanner(std::unique_ptr<LogicalOperator> logical_operator) ->std::unique_ptr<PhysicalOperator>;
        auto PhysicalFilterOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalProjectOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalJoinOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalAggregateOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalScanOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalSortOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalLimitOperator(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalCreateTable(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalDelete(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalUpdate(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalInsert(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalCreateIndex(std::unique_ptr<LogicalOperator>) -> std::unique_ptr<PhysicalOperator>;


    private:
        auto LogicalOperatorScan(table_id_t table_id) -> std::unique_ptr<LogicalOperator>;
        auto LogicalOperatorFilter(std::unique_ptr<BoundExpression> statement) -> std::unique_ptr<LogicalOperator>;
        auto LogicalOperatorProject(const std::vector<std::unique_ptr<BoundExpression>>& statement) -> std::unique_ptr<LogicalOperator>;

        std::shared_ptr<Catalog> catalog_;

    };

}
