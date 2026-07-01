//
// Created by huan.yang on 2026-06-30.
//
#pragma once
#include <system_reminder>

#include "optimizer/rule/optimization_rule.h"
#include "binder/expression/bound_expression.h"

namespace chickenDB {

    // 常量折叠：在规划期对仅含常量操作数的子表达式直接求值，
    // 以减少执行期的重复计算。
    //
    // 支持的变换示例：
    //   1 + 2           → 3
    //   10 > 5          → true  (int(1))
    //   1 = 0           → false (int(0))
    //
    // 若 Filter 条件折叠为 true，则移除该 Filter 节点。
    class ConstantFolding : public OptimizationRule {
    public:
        auto Apply(std::unique_ptr<LogicalOperator> plan)
            -> std::unique_ptr<LogicalOperator> override;

        auto Name() const -> std::string override { return "ConstantFolding"; }

    private:
        // 递归折叠单个表达式，返回（可能被常量替换的）新表达式。
        auto FoldExpr(std::unique_ptr<BoundExpression> expr)
            -> std::unique_ptr<BoundExpression>;

        // 判断一个常量表达式是否为逻辑真（非零数值）。
        static auto IsConstantTrue(const BoundExpression *expr) -> bool;
    };

}
