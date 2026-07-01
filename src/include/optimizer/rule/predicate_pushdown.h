//
// Created by huan.yang on 2026-06-30.
//
#pragma once
#include <system_reminder>
#include <vector>

#include "optimizer/rule/optimization_rule.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"

namespace chickenDB {

    // 谓词下推：将 Filter 节点尽量推向叶子（Scan），
    // 以便尽早过滤行，减少上层算子的输入规模。
    //
    // 支持的变换：
    //   1. Filter → Project → X
    //        ⟹ Project → Filter → X（过滤先于投影）
    //
    //   2. Filter(cond) → Join(left, right)
    //        将 cond 按 AND 拆分，纯引用左侧表的谓词推入左子树，
    //        纯引用右侧表的谓词推入右子树，跨侧谓词留在 Join 上方。
    class PredicatePushdown : public OptimizationRule {
    public:
        auto Apply(std::unique_ptr<LogicalOperator> plan)
            -> std::unique_ptr<LogicalOperator> override;

        auto Name() const -> std::string override { return "PredicatePushdown"; }

    private:
        // 对整棵子树递归应用谓词下推，并将 extra_preds 一并下推。
        auto PushdownPlan(std::unique_ptr<LogicalOperator> plan,
                          std::vector<std::unique_ptr<BoundExpression>> extra_preds)
            -> std::unique_ptr<LogicalOperator>;

        // 如果 preds 非空，在 plan 上方包一层 LogicalFilter。
        static auto WrapWithFilter(std::unique_ptr<LogicalOperator> plan,
                                   std::vector<std::unique_ptr<BoundExpression>> preds)
            -> std::unique_ptr<LogicalOperator>;

        // 将 AND 链拆成独立谓词列表，便于逐条下推。
        static auto SplitAnd(std::unique_ptr<BoundExpression> expr,
                             std::vector<std::unique_ptr<BoundExpression>> &out) -> void;

        // 将谓词列表用 AND 组合成一棵树；列表为空时返回 nullptr。
        static auto CombineAnd(std::vector<std::unique_ptr<BoundExpression>> preds)
            -> std::unique_ptr<BoundExpression>;

        // 收集表达式中所有列引用的 table_id。
        static auto CollectTableRefs(const BoundExpression *expr,
                                     std::vector<table_id_t> &out) -> void;

        // 判断表达式是否只引用 allowed 中的表（无列引用也返回 true）。
        static auto OnlyRefs(const BoundExpression *expr,
                             const std::vector<table_id_t> &allowed) -> bool;
    };

}
