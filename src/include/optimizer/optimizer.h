//
// Created by huan.yang on 2026-06-30.
//
#pragma once
#include <system_reminder>
#include <memory>
#include <vector>

#include "catalog/catalog.h"
#include "optimizer/rule/optimization_rule.h"
#include "planner/logical/logical_operator.h"

namespace chickenDB {

    // 基于规则的逻辑计划优化器（Rule-Based Optimizer，RBO）。
    //
    // 位于流水线中 Planner::CreateLogicalPlanner 之后、
    // Planner::CreatePhysicalPlanner 之前，对逻辑计划树做等价变换：
    //
    //   SQL → Parser → Binder → LogicalPlanner
    //       → [Optimizer]
    //       → PhysicalPlanner → Executor
    //
    // 内置规则（按应用顺序）：
    //   1. ConstantFolding   — 常量折叠
    //   2. PredicatePushdown — 谓词下推
    class Optimizer {
    public:
        explicit Optimizer(std::shared_ptr<Catalog> catalog);
        ~Optimizer() = default;

        // 对逻辑计划树依次应用所有规则，返回优化后的新树。
        auto Optimize(std::unique_ptr<LogicalOperator> plan)
            -> std::unique_ptr<LogicalOperator>;

    private:
        std::shared_ptr<Catalog> catalog_;
        std::vector<std::unique_ptr<OptimizationRule>> rules_;
    };

}
