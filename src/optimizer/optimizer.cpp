//
// Created by huan.yang on 2026-06-30.
//
#include "optimizer/optimizer.h"

#include "optimizer/rule/constant_folding.h"
#include "optimizer/rule/predicate_pushdown.h"

using namespace chickenDB;

Optimizer::Optimizer(std::shared_ptr<Catalog> catalog)
    : catalog_(std::move(catalog)) {
    // 规则按顺序应用：先常量折叠，再谓词下推。
    // 常量折叠在前可简化谓词，让下推时的谓词更干净。
    rules_.push_back(std::make_unique<ConstantFolding>());
    rules_.push_back(std::make_unique<PredicatePushdown>());
}

auto Optimizer::Optimize(std::unique_ptr<LogicalOperator> plan)
    -> std::unique_ptr<LogicalOperator> {
    for (auto &rule : rules_) {
        plan = rule->Apply(std::move(plan));
    }
    return plan;
}
